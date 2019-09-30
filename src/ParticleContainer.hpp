#ifndef PARTICLE_CONTAINER_HPP
#define PARTICLE_CONTAINER_HPP
#include "Particle.hpp"
#include <vector>
#include <random>

class ParticleContainer {
  public:
    ParticleContainer();
    ParticleContainer(const int global_id_start_, const double ave_crossings, const int migrate_chance_, const int seed);

    // Access operators
    inline Particle& operator[](const int idx);
    inline const Particle& operator[](const int idx) const;

    // Adds a newly created particle with unique ID to the
    // end of the vector. Returns the index
    int addParticle();
    
    // Adds an existing particle to the end of the vector
    // Returns the index
    int addParticle(const Particle& p);
    
    // Marks a particle for migration
    void migrateParticle(const int idx);
    
    // Set particles an individual number of moves based on some distribution
    // Currently this is a poisson distribution to get the number of crossings
    // Moves is then crossings + 1
    void setNumMoves();
    
    // Does the 'move', and marks particles for migration
    void moveKernel(const int start, const int end, const int part_ns);

    // Takes the particle list and compacts it by removing all particles
    // that are migrated
    void compactList();

    // Dump state of all particles for debugging
    void dumpParticles();
    
    // Reserve a set amount of slots
    // Returns new capacity
    int reserve(const int amount);

    // Reserve an additional amount of slots if capacity < size + amount
    // Capacity will be at least size + amount
    // Returns new capacity
    int reserveAdditional(const int amount);
    
    // Get the number of particles we have
    int size();

    // Get the current max capacity of the container
    int capacity();

    // Query total time spent moving particles so far (sum of sleeps)
    double getTimeMoved();
  
  private:
    std::vector<Particle> particles;
    std::vector<int> migrate_list;
    int global_id;
    int migrate_chance;
    double total_seconds;

    std::default_random_engine engine;
    std::default_random_engine migrate_engine;
    std::poisson_distribution<int> distribution;
    std::uniform_int_distribution<> migrate_distribution;
};

#endif
