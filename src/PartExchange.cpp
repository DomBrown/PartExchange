#include <mpi.h>
#include <vt/transport.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>

#include "yaml-cpp/yaml.h"

#include "ParticleContainer.hpp"
#include "OutputWriter.hpp"

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

  int initial_total = parts_per_rank * nranks;

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

  int my_final_count = particles.size();
  int total_count = 0;
  MPI_Reduce(&my_final_count, &total_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  if(rank == 0) {
    std::string fname;
    if(input_deck["Statistics File Name"]) {
      fname = input_deck["Statistics File Name"].as<std::string>();
    }
    
    OutputWriter writer(fname);
    
    writer.writeStatistics("Computation", move_times);

    writer.writeStatistics("Migration", comm_times);

    if(total_count != initial_total) {
      std::cout << "ERROR: Beginning and final particle counts do not match!" << std::endl;
    }
  }

  MPI_Finalize();

  return 0;
}
