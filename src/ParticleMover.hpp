#ifndef PARTICLE_MOVER_HPP
#define PARTICLE_MOVER_HPP
#include "Particle.hpp"
#include "ParticleContainer.hpp"
#include <vt/transport.h>
#include <vector>
#include <random>
#include <utility>
#include <cassert>

struct NullMsg : vt::Message {};

class ParticleMover {
  public:
    ParticleMover() = delete;
    ParticleMover(ParticleContainer& particles, const int move_part_ns_, const double ave_crossings, const int migrate_chance_, const int seed);

    // Marks a particle for migration
    void migrateParticle(const int idx);
    
    // Set particles an individual number of moves based on some distribution
    // Currently this is a poisson distribution to get the number of crossings
    // Moves is then crossings + 1
    void setNumMoves();

    void moveParticles();

    // Does the 'move', and marks particles for migration
    void moveKernel(const int start, const int end);

    int doMigration(int& next_start);

    // Takes the particle list and compacts it by removing all particles
    // that are migrated
    void compactList();

    // Query total time spent moving particles so far (sum of sleeps)
    double getTimeMoved();

    void setupNeighbours();

    void moveHandler(NullMsg *msg);

    // Handler to be called when we recv particles
    void particleMigrationHandler(ParticleMsg *msg);

    int size();
  
  private:
    ParticleContainer& particles;
    int particle_start_idx;
    std::vector<int> migrate_list;
    int move_part_ns;
    int global_id;
    int migrate_chance;
    double total_seconds;

    std::mt19937 engine;
    std::mt19937 migrate_engine;
    std::mt19937 neighbour_engine;
    std::poisson_distribution<int> distribution;
    std::uniform_int_distribution<> migrate_distribution;
    std::uniform_int_distribution<> neighbour_distribution;

    int rank;
    int nranks;
    std::vector<int> neighbours;
    std::vector<std::pair<int,int>> particle_dests;
};

#endif
