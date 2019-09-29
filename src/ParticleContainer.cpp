#include "ParticleContainer.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cassert>

ParticleContainer::ParticleContainer() : global_id(0), migrate_chance(10), distribution(std::poisson_distribution<int>(1.0)) {
  engine.seed(240694);
  migrate_engine.seed(240694);
  migrate_distribution = std::uniform_int_distribution<>(1, 100);
  std::cout << "Setting default crossing average (1.0) and seed!" << std::endl;
  std::cout << "Setting default migrate chance of 10%!" << std::endl;
}

ParticleContainer::ParticleContainer(const int global_id_start_, const int ave_crossings, const int migrate_chance_, const int seed) : global_id(global_id_start_), migrate_chance(migrate_chance_), distribution(std::poisson_distribution<int>(ave_crossings)) {
  engine.seed(seed);
  migrate_engine.seed(seed);
  migrate_distribution = std::uniform_int_distribution<>(1, 100);
  std::cout << "Setting average crossings: " << ave_crossings << ", seed: " << seed << std::endl;
  std::cout << "Setting migration chance: " << migrate_chance << "%!" << std::endl;
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

void ParticleContainer::moveKernel(const int start, const int end, const int part_ns) {
  int total_ns = 0;
  for(int iPart = start; iPart < end; iPart++) {
    
    while(particles[iPart].num_moves > 0) {
      particles[iPart].num_moves--;
      total_ns += part_ns;
      
      if(particles[iPart].num_moves > 0) { // If we only had one move, there was no crossing, so no migration either
        const int migrate_roll = migrate_distribution(migrate_engine);
        if(migrate_roll <= migrate_chance) {
          migrate_list.push_back(iPart); // FIXME, this should be idx in vector
          break; // Move on as we're done with this one
        }
      }
    }
  }
  std::this_thread::sleep_for(std::chrono::nanoseconds(total_ns));
  
  //double sec = total_ns / 1e9;
  //std::cout << "Move time: " << sec << std::endl;
  //for(auto& m : migrate_list) std::cout << m << std::endl; // For now dump list for debug
}

void ParticleContainer::compactList() {
  int rm_index = 0;
  particles.erase(
    std::remove_if(std::begin(particles), std::end(particles), [&](Particle& part) {
      if(migrate_list.size() != rm_index && &part - &particles[0] == migrate_list[rm_index]) {
        rm_index++;
        return true;
      }
      return false;
    }),
    std::end(particles)
  );

  migrate_list.clear();
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
