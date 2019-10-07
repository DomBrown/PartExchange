#include <mpi.h>
#include <vt/transport.h>

#include <iostream>

#include "yaml-cpp/yaml.h"

#include "ParticleContainer.hpp"

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
  const int migration_chance = input_deck["Migration Chance"].as<int>(); // FIXME This should get set to zero if nranks == 0

  int parts_per_rank = nparticles / nranks;

  int start = parts_per_rank * rank;

  ParticleContainer particles(start, ave_crossings, migration_chance, rng_seed);

  particles.reserve(parts_per_rank);

  for(int i = 0; i < parts_per_rank; i++)
    particles.addParticle();

  // Wait for everyone to load their particles
  MPI_Barrier(MPI_COMM_WORLD);

  for(int step = 0; step < nsteps; step++) {
    if(rank == 0) {
      std::cout << "Step " << step << std::endl;
    }
    
    // Setup for move, assign each particle a move count based on the
    // distribution we set up
    particles.setNumMoves();
    
    int total_sent = 0;
    int start = 0;

    do {
      // Do the move
      particles.moveKernel(start, particles.size(), move_part_ns);
      
      total_sent = particles.doMigration(start);
    } while(total_sent > 0);

  }
  
  MPI_Barrier(MPI_COMM_WORLD);
  if(rank == 0) {
    std::cout << "Rank " << rank << " Total Move time: " << particles.getTimeMoved() << std::endl;
  }

  std::cout << "Rank " << rank << " Final Count " << particles.size() << std::endl;

  MPI_Finalize();

  return 0;
}
