#include "ParticleContainer.hpp"
#include <mpi.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cassert>

ParticleContainer::ParticleContainer() : global_id(0), migrate_chance(10), total_seconds(0.0), distribution(std::poisson_distribution<int>(1.0)) {
  engine.seed(240694);
  migrate_engine.seed(240694);
  migrate_distribution = std::uniform_int_distribution<>(1, 100);
  std::cout << "Setting default crossing average (1.0) and seed!" << std::endl;
  std::cout << "Setting default migrate chance of 10%!" << std::endl;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
}

ParticleContainer::ParticleContainer(const int global_id_start_, const double ave_crossings, const int migrate_chance_, const int seed) : global_id(global_id_start_), migrate_chance(migrate_chance_), total_seconds(0.0), distribution(std::poisson_distribution<int>(ave_crossings)) {
  engine.seed(seed);
  migrate_engine.seed(seed);
  migrate_distribution = std::uniform_int_distribution<>(1, 100);
  std::cout << "Setting average crossings: " << ave_crossings << ", seed: " << seed << std::endl;
  std::cout << "Setting migration chance: " << migrate_chance << "%!" << std::endl;
  
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
}

Particle& ParticleContainer::operator[](const int idx) {
  assert(("Particle container bounds error!", (idx < particles.size()) && (idx >= 0)));
  return particles[idx];
}

const Particle& ParticleContainer::operator[](const int idx) const {
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
  unsigned long total_ns = 0;
  for(int iPart = start; iPart < end; iPart++) {
    
    while(particles[iPart].num_moves > 0) {
      particles[iPart].num_moves--;
      total_ns += part_ns;
      
      if(particles[iPart].num_moves > 0) { // If we only had one move, there was no crossing, so no migration either
        const int migrate_roll = migrate_distribution(migrate_engine);
        if(migrate_roll <= migrate_chance) {
          migrate_list.push_back(iPart);
          break; // Move on as we're done with this one
        }
      }
    }
  }
  std::this_thread::sleep_for(std::chrono::nanoseconds(total_ns));
  
  double sec = total_ns / 1e9;
  total_seconds += sec;
}


int ParticleContainer::doMigration(int& next_start) {
  int neighbour = (rank == 0) ? 1 : 0;
  int n_to_send = migrate_list.size();

  int nrecv = 0;
  // Work out how many particles we are expecting
  MPI_Sendrecv(
    &n_to_send, 1, MPI_INT, neighbour, 0,
    &nrecv, 1, MPI_INT, neighbour, 0,
    MPI_COMM_WORLD,
    MPI_STATUS_IGNORE
  );

  std::vector<Particle> sendbuf;
  std::vector<Particle> recvbuf;
  sendbuf.reserve(n_to_send);
  recvbuf.resize(nrecv);

  // Pack send buffer
  for(int i = 0; i < migrate_list.size(); i++) {
    int idx = migrate_list[i];
    sendbuf.push_back(particles[idx]);
  }

  // Migrate the particles
  MPI_Sendrecv(
    sendbuf.data(), n_to_send*sizeof(Particle), MPI_CHAR, neighbour, 0,
    recvbuf.data(), nrecv*sizeof(Particle), MPI_CHAR, neighbour, 0,
    MPI_COMM_WORLD,
    MPI_STATUS_IGNORE
  );
  
  compactList();
  next_start = particles.size();

  // Ensure there is enough space
  reserveAdditional(nrecv);

  for(int i = 0; i < recvbuf.size(); i++) {
    particles.push_back(recvbuf[i]);
  }

  int total_send = 0;
  MPI_Allreduce(
    &n_to_send,
    &total_send,
    1,
    MPI_INT,
    MPI_SUM,
    MPI_COMM_WORLD
  );

  return total_send;
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
  
  std::cout << "****** Begin Rank " << rank << " Particle Dump ******" << std::endl;
  for(auto &p : particles) {
    std::cout << "ID: " << p.id << "\tNumMoves: " << p.num_moves << std::endl;
  }
  std::cout << "******* End Rank " << rank << " Particle Dump *******" << std::endl;
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

double ParticleContainer::getTimeMoved() {
  return total_seconds;
}
