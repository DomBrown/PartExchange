#include "ParticleContainer.hpp"
#include <mpi.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

ParticleContainer::ParticleContainer() : 
  global_id(0) {}

ParticleContainer::ParticleContainer(const int global_id_start_) :
  global_id(global_id_start_) {}


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

void ParticleContainer::compactList(std::vector<int>& migrate_list) {
  auto back = particles.size() - 1;
  auto last_valid = particles.size() - migrate_list.size();

  for(int i = 0; i < migrate_list.size(); i++) {
    auto which_dead = migrate_list[i];
    
    if(which_dead >= last_valid)
      continue;
    
    while(back > which_dead && (particles[back].dead))
      back--;
    
    particles[which_dead] = particles[back];
    back--;
  }
  
  int new_size = particles.size() - migrate_list.size();
  
  particles.resize(new_size);
  migrate_list.clear();
}

void ParticleContainer::dumpParticles(const int rank) {
  
  std::cout << "****** Begin Rank " << rank << " Particle Dump ******" << std::endl;
  for(auto &p : particles) {
    std::cout << "ID: " << p.id << "\tNumMoves: " << p.num_moves << "\tDead: " << p.dead << std::endl;
  }
  std::cout << "******* End Rank " << rank << " Particle Dump *******" << std::endl;
}

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
