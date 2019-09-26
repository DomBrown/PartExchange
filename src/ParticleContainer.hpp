#ifndef PARTICLE_CONTAINER_HPP
#define PARTICLE_CONTAINER_HPP
#include "Particle.hpp"
#include <vector>
#include <random>

class ParticleContainer {
  public:
    ParticleContainer();
    ParticleContainer(const unsigned int global_id_start_);

    // Access operators
    inline Particle& operator[](const unsigned int idx);
    inline const Particle& operator[](const unsigned int idx) const;

    // Adds a newly created particle with unique ID to the
    // end of the vector. Returns the index
    unsigned int addParticle();
    
    // Adds an existing particle to the end of the vector
    // Returns the index
    unsigned int addParticle(const Particle& p);
    
    // Marks a particle for migration
    void migrateParticle(const unsigned int idx);
    
    // Set particles an individual number of moves based on some distribution
    void setNumMoves();

    // Dump state of all particles for debugging
    void dumpParticles();
    
    // Reserve a set amount of slots
    // Returns new capacity
    unsigned int reserve(const unsigned int amount);

    // Reserve an additional amount of slots if capacity < size + amount
    // Capacity will be at least size + amount
    // Returns new capacity
    unsigned int reserveAdditional(const unsigned int amount);
    
    // Get the number of particles we have
    unsigned int size();

    // Get the current max capacity of the container
    unsigned int capacity();
  
  private:
    std::vector<Particle> particles;
    unsigned int global_id;
};

#endif
