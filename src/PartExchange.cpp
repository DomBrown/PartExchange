#include <mpi.h>
#include <vt/transport.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

#include "yaml-cpp/yaml.h"

#include "ParticleContainer.hpp"

// Takes a vector of times, and dumps out some useful stats
void getStatistics(const std::vector<double>& times) {
  double average, total = 0.;
  double max = -1e99;
  double min = 1e99;

  for(auto& t : times) {
    total += t;
    min = std::min(min, t);
    max = std::max(max, t);
  }

  average = total / times.size();

  double sum_devs = 0.;
  for(auto& t : times) {
    sum_devs += pow((t - average), 2);
  }

  sum_devs /= times.size();
  double stdev = sqrt(sum_devs);

  std::cout << "Min: " << min << std::endl;
  std::cout << "Average: " << average << std::endl;
  std::cout << "Max: " << max << std::endl;
  std::cout << "StDev: " << stdev << std::endl;
}

int main(int argc, char** argv) {

  MPI_Init(NULL, NULL);

  int nranks, rank;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nranks);

  YAML::Node input_deck;

  if(argc < 2) {
    std::cout << "Provide an input YAML file!" << std::endl;
    return 1;
  } else {
    input_deck = YAML::LoadFile(argv[1]);
  }

  const int nsteps = input_deck["Timesteps"].as<int>();
  const int nparticles = input_deck["Particle Count"].as<int>();
  const double ave_crossings = input_deck["Average Crossings"].as<double>();
  const int rng_seed = input_deck["Crossing RNG Seed"].as<int>() + rank; // Make sure each rank seed is different or else we have problems
  const int move_part_ns = input_deck["Move Particle Nanoseconds"].as<int>();
  int migration_chance;

  // Never migrate if ranks < 2
  if(nranks > 1) {
    migration_chance = input_deck["Migration Chance"].as<int>();
  } else {
    migration_chance = 0;
    std::cout << "Running with only 1 rank: Forcing migration chance = 0!" << std::endl;
  }

  int parts_per_rank = nparticles / nranks;

  int start = parts_per_rank * rank;

  ParticleContainer particles(start, ave_crossings, migration_chance, rng_seed);

  particles.reserve(parts_per_rank); // Maybe reserve some extra to prevent reallocs later??

  for(int i = 0; i < parts_per_rank; i++)
    particles.addParticle();

  // Wait for everyone to load their particles
  MPI_Barrier(MPI_COMM_WORLD);

  double commtime = 0.;

  for(int step = 0; step < nsteps; step++) {
    if(rank == 0) {
      std::cout << "Step " << step << std::endl;
    }
    
    // Setup for move, assign each particle a move count based on the
    // distribution we set up
    particles.setNumMoves();

    // Finish setting up before we do the work for this step
    MPI_Barrier(MPI_COMM_WORLD);
    
    int total_sent = 0;
    int start = 0;
    
    do {
      // Do the move
      particles.moveKernel(start, particles.size(), move_part_ns);
      
      double commstart = MPI_Wtime();

      total_sent = particles.doMigration(start);

      commtime += (MPI_Wtime() - commstart);
    } while(total_sent > 0);

  }
  
  // Make sure everyone is done before we process the timing data
  MPI_Barrier(MPI_COMM_WORLD);

  std::cout << "Rank " << rank << " Final Count " << particles.size() << std::endl;

  std::vector<double> comm_times, move_times;
  comm_times.resize(nranks);
  move_times.resize(nranks);

  // Gather up times from all ranks and generate statistics
  MPI_Gather(&commtime, 1, MPI_DOUBLE, comm_times.data(), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  double my_move_time = particles.getTimeMoved();
  MPI_Gather(&my_move_time, 1, MPI_DOUBLE, move_times.data(), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  if(rank == 0) {
    std::cout << "***** Migration Statistics *****" << std::endl;
    getStatistics(comm_times);
    std::cout << std::endl;

    std::cout << "****** Compute Statistics ******" << std::endl;
    getStatistics(move_times);
  }

  MPI_Finalize();

  return 0;
}
