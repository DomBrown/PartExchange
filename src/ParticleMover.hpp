#ifndef PARTICLE_MOVER_HPP
#define PARTICLE_MOVER_HPP
#include "Particle.hpp"
#include "ParticleContainer.hpp"
#include "CustomReducer.hpp"

#include <vt/transport.h>
#include <vector>
#include <random>
#include <utility>
#include <cassert>
#include <cstdlib>

using IndexType = vt::IdxType1D<std::size_t>;

struct NullMsg : vt::Message {};

class ParticleMover : public vt::Collection<ParticleMover, IndexType> {
  public:
    
    struct NullMsg : vt::CollectionMessage<ParticleMover> {};

    struct DumpMsg : vt::CollectionMessage<ParticleMover> {
      DumpMsg() = default;

      DumpMsg(int rank_) : rank(rank_) {}

      public:
        int rank;
    };

		struct ParticleMsg : vt::CollectionMessage<ParticleMover> {
      ParticleMsg() = default;

      // Add a serialiser that will serialise the particle vector
      template <typename SerializerT>
      void serialize(SerializerT& s) {
        s | particles;
      }

      public:
        std::vector<Particle> particles;
    };
    
    ParticleMover() = default;
    ParticleMover(const int num_particles, const int start, const int move_part_ns_, const double ave_crossings, const int migrate_chance_, const int seed);

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

    void setNumMovesHandler(NullMsg *msg);

    void particleDumpHandler(DumpMsg *msg);

    void printParticleCountsHandler(NullMsg *msg);

    int size();
  
  private:
    ParticleContainer particles;
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
