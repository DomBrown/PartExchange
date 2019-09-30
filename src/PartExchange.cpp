#include <mpi.h>
#include <vt/transport.h>

#include <iostream>

#include "yaml-cpp/yaml.h"

#include "ParticleContainer.hpp"

int main(int argc, char** argv) {
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
  const int rng_seed = input_deck["Crossing RNG Seed"].as<int>();
  const int move_part_ns = input_deck["Move Particle Nanoseconds"].as<int>();
  const int migration_chance = input_deck["Migration Chance"].as<int>();


  ParticleContainer particles(0, ave_crossings, migration_chance, rng_seed);

  particles.reserve(nparticles);

  for(int i = 0; i < nparticles; i++)
    particles.addParticle();

  for(int step = 0; step < nsteps; step++) {
    std::cout << "Step " << step << std::endl;
    
    // Setup for move, assign each particle a move count based on the
    // distribution we set up
    particles.setNumMoves();
    
    //particles.dumpParticles();
    // Do the move
    particles.moveKernel(0, particles.size(), move_part_ns);
    particles.compactList();
    //particles.dumpParticles();
  }

  std::cout << "Total Move time: " << particles.getTimeMoved() << std::endl;

  return 0;
}
