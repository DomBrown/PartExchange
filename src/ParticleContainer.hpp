#ifndef PARTICLE_CONTAINER_HPP
#define PARTICLE_CONTAINER_HPP
#include "Particle.hpp"
#include <vt/transport.h>
#include <vector>
#include <random>
#include <utility>
#include <cassert>
#include <cstdlib>

using IndexType = vt::IdxType1D<std::size_t>;

class ParticleContainer {
  public:
    ParticleContainer();
    ParticleContainer(const int global_id_start_);

    // Access operators
    inline Particle& operator[](const int idx) {
      assert(("Particle container bounds error!", (idx < particles.size()) && (idx >= 0)));
      return particles[idx];
    }

    inline const Particle& operator[](const int idx) const {
      assert(("Particle container bounds error!", (idx < particles.size()) && (idx >= 0)));
      return particles[idx];
    }

    // Adds a newly created particle with unique ID to the
    // end of the vector. Returns the index
    int addParticle();
    
    // Adds an existing particle to the end of the vector
    // Returns the index
    int addParticle(const Particle& p);

    // Takes the particle list and compacts it by removing all particles
    // that are migrated
    void compactList(std::vector<int> &migrate_list);

    // Dump state of all particles for debugging
    void dumpParticles(const int rank);
    
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

  private:
    std::vector<Particle> particles;
    int global_id;
};

#endif
