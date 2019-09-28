#include "ParticleContainer.hpp"
#include <iostream>
#include <cassert>

ParticleContainer::ParticleContainer() : global_id(0), distribution(std::poisson_distribution<int>(1.0)) {
  engine.seed(240694);
  std::cout << "Setting default crossing average and seed!" << std::endl;
}

ParticleContainer::ParticleContainer(const int global_id_start_, const int ave_crossings, const int seed) : global_id(global_id_start_), distribution(std::poisson_distribution<int>(ave_crossings)) {
  engine.seed(seed);
  std::cout << "Setting average crossings: " << ave_crossings << ", seed: " << seed << std::endl;
}

inline Particle& ParticleContainer::operator[](const int idx) {
  assert(("Particle container bounds error!", (idx < particles.size()) && (idx >= 0)));
  return particles[idx];
}

inline const Particle& ParticleContainer::operator[](const int idx) const {
  assert(("Particle container bounds error!", (idx < particles.size()) && (idx >= 0)));
  return particles[idx];
}

int ParticleContainer::addParticle() {
  particles.push_back(Particle(global_id++));
  int loc = particles.size() - 1;
  return loc;
}

int ParticleContainer::addParticle(const Particle& p) {
  particles.push_back(p);
  int loc = particles.size() - 1;
  return loc;
}

void ParticleContainer::setNumMoves() {
  for(auto &p : particles) {
    int num_crossings = distribution(engine);
    p.num_moves = num_crossings + 1;
  }
}

void ParticleContainer::dumpParticles() {
  std::cout << "****** Begin Particle Dump ******" << std::endl;
  for(auto &p : particles) {
    std::cout << "ID: " << p.id << "\tNumMoves: " << p.num_moves << std::endl;
  }
  std::cout << "******* End Particle Dump *******" << std::endl;
}

void ParticleContainer::migrateParticle(const int idx) {}

int ParticleContainer::reserve(const int amount) {
  particles.reserve(amount);
  return particles.capacity();
}

int ParticleContainer::reserveAdditional(const int amount) {
  if(particles.capacity() < (particles.size() + amount))
    particles.reserve(particles.capacity() + amount);
  
  return particles.capacity();
}

int ParticleContainer::capacity() {
  return particles.capacity();
}

int ParticleContainer::size() {
  return particles.size();
}
