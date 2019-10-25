#include <mpi.h>
#include <vt/transport.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <string>

#include "yaml-cpp/yaml.h"

#include "ParticleContainer.hpp"
#include "OutputWriter.hpp"

int distributeParticles(const int nparticles, const int my_rank, const int nranks, const double stdev, const int rng_seed) {
  
  std::vector<int> rank_counts;
  rank_counts.resize(nranks);
  
  // Root generates the particle counts, then bcast them out
  if(my_rank == 0) {
    std::mt19937 pseudorandom_generator(rng_seed);

    auto maxrank = nranks - 1;

    double mean = nparticles/nranks;
    auto min_allowed = 0;
    auto max_allowed = nparticles;
    std::normal_distribution<> distribution{mean, stdev};

    for(int rank = 0; rank < nranks; rank++) {
      int amount = 0.; 
      if(rank == maxrank) {
        amount = max_allowed;
      } else {
        // Resample until we get a valid value
        do {
          amount = std::round(distribution(pseudorandom_generator));
        } while(!((amount > min_allowed) && (amount < max_allowed)));
      }   
      
      rank_counts[rank] = amount;
      max_allowed -= amount;
      std::cout << "Rank " << rank << " has " << rank_counts[rank] << std::endl;
    }
  }

  MPI_Bcast(rank_counts.data(), nranks, MPI_INT, 0, MPI_COMM_WORLD);

  return rank_counts[my_rank];
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
  const double dist_stdev = input_deck["Particle Distribution"]["Standard Deviation"].as<double>();

  // Never migrate if ranks < 2
  if(nranks > 1) {
    migration_chance = input_deck["Migration Chance"].as<int>();
  } else {
    migration_chance = 0;
    std::cout << "Running with only 1 rank: Forcing migration chance = 0!" << std::endl;
  }

  ParticleContainer particles(0, ave_crossings, migration_chance, rng_seed);
  
  const int my_initial_total = distributeParticles(nparticles, rank, nranks, dist_stdev, rng_seed);

  particles.reserve(my_initial_total); 

  for(int i = 0; i < my_initial_total; i++)
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

    if(total_count != nparticles) {
      std::cout << "ERROR: Beginning and final particle counts do not match!" << std::endl;
    }
  }

  MPI_Finalize();

  return 0;
}
