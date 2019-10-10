#include "ParticleContainer.hpp"
#include <mpi.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

ParticleContainer::ParticleContainer() : global_id(0), migrate_chance(10), total_seconds(0.0), distribution(std::poisson_distribution<int>(1.0)) {
  engine.seed(240694);
  migrate_engine.seed(240694);
  migrate_distribution = std::uniform_int_distribution<>(1, 100);
  std::cout << "Setting default crossing average (1.0) and seed!" << std::endl;
  std::cout << "Setting default migrate chance of 10%!" << std::endl;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nranks);

  setupNeighbours();
  particle_dests.reserve(100);
}

ParticleContainer::ParticleContainer(const int global_id_start_, const double ave_crossings, const int migrate_chance_, const int seed) : global_id(global_id_start_), migrate_chance(migrate_chance_), total_seconds(0.0), distribution(std::poisson_distribution<int>(ave_crossings)) {
  engine.seed(seed);
  migrate_engine.seed(seed);
  migrate_distribution = std::uniform_int_distribution<>(1, 100);
  std::cout << "Setting average crossings: " << ave_crossings << ", seed: " << seed << std::endl;
  std::cout << "Setting migration chance: " << migrate_chance << "%!" << std::endl;
  
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nranks);

  setupNeighbours();
  particle_dests.reserve(100);
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
          int neighbour_idx = iPart % neighbours.size();  // Change this to a rand dist. later
          particles[iPart].dead = 1;
          migrate_list.push_back(iPart);
          particle_dests.emplace_back(iPart, neighbour_idx);
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
  const int num_neighbours = neighbours.size();
  int my_total_send = 0;
  int my_total_recv = 0;
  
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

  std::vector<int> my_recv_counts(num_neighbours);

  std::vector<MPI_Status> status(2*num_neighbours);
  std::vector<MPI_Request> request(2*num_neighbours);

  // Work out how many particles we are expecting
  for(int i = 0; i < num_neighbours; i++) {
    MPI_Isend(my_send_counts.data()+i, 1, MPI_INT, neighbours[i], 0, MPI_COMM_WORLD, request.data()+(2*i));
    MPI_Irecv(my_recv_counts.data()+i, 1, MPI_INT, neighbours[i], 0, MPI_COMM_WORLD, request.data()+(2*i+1));
    my_total_send += my_send_counts[i];
  }
  
  MPI_Waitall(2*num_neighbours, request.data(), status.data());

  for(int i = 0; i < my_recv_counts.size(); i++)
    my_total_recv += my_recv_counts[i];

  std::vector<Particle> recvbuf;
  recvbuf.resize(my_total_recv);

  // Migrate the particles
  int recv_start = 0;
  for(int i = 0; i < num_neighbours; i++) {
    MPI_Isend(my_send_bufs[i].data(), my_send_counts[i]*sizeof(Particle), MPI_CHAR, neighbours[i], 0, MPI_COMM_WORLD, request.data()+(2*i));
    MPI_Irecv(recvbuf.data()+recv_start, my_recv_counts[i]*sizeof(Particle), MPI_CHAR, neighbours[i], 0, MPI_COMM_WORLD, request.data()+(2*i+1));
    recv_start += my_recv_counts[i];
  }

  // Do this here to hide comms latency
  compactList();
  particle_dests.clear();
  
  MPI_Waitall(2*num_neighbours, request.data(), status.data());
  
  next_start = particles.size();

  // Ensure there is enough space, only expands if needed
  reserveAdditional(my_total_recv);

  for(int i = 0; i < recvbuf.size(); i++) {
    recvbuf[i].dead = 0;
    particles.push_back(recvbuf[i]);
  }

  int total_send = 0;
  MPI_Allreduce(
    &my_total_send,
    &total_send,
    1,
    MPI_INT,
    MPI_SUM,
    MPI_COMM_WORLD
  );

  return total_send;
}

void ParticleContainer::compactList() {
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

void ParticleContainer::dumpParticles() {
  
  std::cout << "****** Begin Rank " << rank << " Particle Dump ******" << std::endl;
  for(auto &p : particles) {
    std::cout << "ID: " << p.id << "\tNumMoves: " << p.num_moves << "\tDead: " << p.dead << std::endl;
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

// I know this is horrible but it's only temporary
void ParticleContainer::setupNeighbours() {
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
