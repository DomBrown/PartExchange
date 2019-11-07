#include "ParticleContainer.hpp"
#include "ParticleMover.hpp"
#include <mpi.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

ParticleMover::ParticleMover(ParticleContainer& particles_, const int move_part_ns_, const double ave_crossings, const int migrate_chance_, const int seed) :
  particles(particles_), particle_start_idx(0), move_part_ns(move_part_ns_), migrate_chance(migrate_chance_),
  total_seconds(0.0), distribution(std::poisson_distribution<int>(ave_crossings)) {

  rank = vt::theContext()->getNode();
  nranks = vt::theContext()->getNumNodes();

  setupNeighbours();

  engine.seed(seed);
  migrate_engine.seed(seed);
  neighbour_engine.seed(seed);

  migrate_distribution = std::uniform_int_distribution<>(1, 100);
  neighbour_distribution = std::uniform_int_distribution<>(0, neighbours.size() - 1);

  particle_dests.reserve(100);
}

void ParticleMover::setNumMoves() {
  particle_start_idx = 0;
  for(int i = 0; i < particles.size(); i++) {
    int num_crossings = distribution(engine);
    particles[i].num_moves = num_crossings + 1;
  }
}

void ParticleMover::moveParticles() {
  moveKernel(particle_start_idx, particles.size());
  
  // Migration starts here
  const int num_neighbours = neighbours.size();
  std::vector<int> my_send_counts(neighbours.size());
  std::vector<std::vector<Particle>> my_send_bufs(num_neighbours);

  // Count how many we are sending, and pack them up
  for(int i = 0; i < particle_dests.size(); i++) {
    int neigh_idx = particle_dests[i].second;

    my_send_counts[neigh_idx]++;
  }

  for(int i = 0; i < num_neighbours; i++) {
    my_send_bufs[i].reserve(my_send_counts[i]);
  }
  
  for(int i = 0; i < particle_dests.size(); i++) {
    int iPart = particle_dests[i].first;
    int neigh_idx = particle_dests[i].second;
    my_send_bufs[neigh_idx].push_back(particles[iPart]);
  }

  particles.compactList(migrate_list);
  particle_dests.clear();
  particle_start_idx = particles.size();

  auto proxy = vt::theObjGroup()->getProxy(this);

  for(int i = 0; i < num_neighbours; i++) {
    if(my_send_counts[i] > 0) {
      auto msg = vt::makeSharedMessage<ParticleMsg>();
      msg->particles = my_send_bufs[i];
      const vt::NodeType to = neighbours[i];
      
      //fmt::print("Node {} sending {} to {}. Epoch {}\n", vt::theContext()->getNode(), my_send_counts[i], to, vt::theMsg()->getEpoch());
      proxy[to].send<ParticleMsg, &ParticleMover::particleMigrationHandler>(msg);
    }
  }
}

void ParticleMover::moveKernel(const int start, const int end) {
  unsigned long total_ns = 0;
  for(int iPart = start; iPart < end; iPart++) {
    
    while(particles[iPart].num_moves > 0) {
      particles[iPart].num_moves--;
      total_ns += move_part_ns;
      
      if(particles[iPart].num_moves > 0) { // If we only had one move, there was no crossing, so no migration either
        const int migrate_roll = migrate_distribution(migrate_engine);
        if(migrate_roll <= migrate_chance) {
          migrateParticle(iPart);
          break; // Move on as we're done with this one
        }
      }
    }
  }
  std::this_thread::sleep_for(std::chrono::nanoseconds(total_ns));
  
  double sec = total_ns / 1e9;
  total_seconds += sec;
}

void ParticleMover::moveHandler(NullMsg *msg) {
  moveParticles();
}

void ParticleMover::particleMigrationHandler(ParticleMsg *msg) {
  int num_recv = msg->particles.size();
  particles.reserveAdditional(num_recv);

  for(int i = 0; i < num_recv; i++) {
    auto p = msg->particles[i];
    p.dead = 0;
    particles.addParticle(p);
  }

  moveParticles();
}

void ParticleMover::migrateParticle(const int idx) {
  const int neighbour_idx = neighbour_distribution(neighbour_engine); // Send to a rand neighbour
  particles[idx].dead = 1;
  migrate_list.push_back(idx);
  particle_dests.emplace_back(idx, neighbour_idx);
}

double ParticleMover::getTimeMoved() {
  return total_seconds;
}

int ParticleMover::size() {
  return particles.size();
}

// I know this is horrible but it's only temporary
void ParticleMover::setupNeighbours() {
  if(nranks == 1) {
    neighbours.push_back(MPI_PROC_NULL);
  } else if(nranks == 2) { // Avoids recording the same neighbour twice in the 2 rank case
    if(rank == 0) {
      neighbours.push_back(1);
    } else {
      neighbours.push_back(0);
    }
  } else {
    // Assume 1D periodic for now
    if((rank + 1) < nranks) {
      neighbours.push_back(rank + 1);
    } else {
      neighbours.push_back(0);
    }

    if((rank - 1) > -1) {
      neighbours.push_back(rank - 1);
    } else {
      neighbours.push_back(nranks - 1);
    }
  }
}