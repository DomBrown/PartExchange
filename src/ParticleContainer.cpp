#include "ParticleContainer.hpp"
#include <iostream>
#include <cassert>

ParticleContainer::ParticleContainer() : global_id(0) {}

ParticleContainer::ParticleContainer(const unsigned int global_id_start_) : global_id(global_id_start_) {}

inline Particle& ParticleContainer::operator[](const unsigned int idx) {
  assert(("Particle container bounds error!", (idx < particles.size()) && (idx >= 0)));
  return particles[idx];
}

inline const Particle& ParticleContainer::operator[](const unsigned int idx) const {
  assert(("Particle container bounds error!", (idx < particles.size()) && (idx >= 0)));
  return particles[idx];
}

unsigned int ParticleContainer::addParticle() {
  particles.push_back(Particle(global_id++));
  unsigned int loc = particles.size() - 1;
  return loc;
}

unsigned int ParticleContainer::addParticle(const Particle& p) {
  particles.push_back(p);
  unsigned int loc = particles.size() - 1;
  return loc;
}

void ParticleContainer::setNumMoves() {
  std::default_random_engine generator;
  std::poisson_distribution<int> distribution(1);
  for(auto &p : particles) {
    int num_crossings = distribution(generator);
    p.num_moves = num_crossings + 1;
  }
}

void ParticleContainer::dumpParticles() {
  std::cout << "****** Begin Particle Dump ******" << std::endl;
  for(auto &p : particles) {
    std::cout << "ID: " << p.id << " NumMoves: " << p.num_moves << std::endl;
  }
  std::cout << "******* End Particle Dump *******" << std::endl;
}

void ParticleContainer::migrateParticle(const unsigned int idx) {}

unsigned int ParticleContainer::reserve(const unsigned int amount) {
  particles.reserve(amount);
  return particles.capacity();
}

unsigned int ParticleContainer::reserveAdditional(const unsigned int amount) {
  if(particles.capacity() < (particles.size() + amount))
    particles.reserve(particles.capacity() + amount);
  
  return particles.capacity();
}

unsigned int ParticleContainer::capacity() {
  return particles.capacity();
}

unsigned int ParticleContainer::size() {
  return particles.size();
}
