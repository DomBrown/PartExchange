#include <mpi.h>
#include <vt/transport.h>

#include <iostream>
#include <chrono>
#include <thread>

#include "yaml-cpp/yaml.h"

#include "ParticleContainer.hpp"

int calculateCost(const int num_parts, const double cost_per_part) {
  return num_parts * cost_per_part;
}

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
  const int ave_crossings = input_deck["Average Crossings"].as<int>();
  const int rng_seed = input_deck["Crossing RNG Seed"].as<int>();
  const int move_part_ns = input_deck["Move Particle Nanoseconds"].as<int>();
  const double migration_chance = input_deck["Migration Chance"].as<double>();


  ParticleContainer particles(0, ave_crossings, rng_seed);

  particles.reserve(nparticles);

  for(int i = 0; i < nparticles; i++)
    particles.addParticle();

  for(int step = 0; step < nsteps; step++) {
    std::cout << "Step " << step << std::endl;
    
    // Setup for move
    particles.setNumMoves();

    // Do the move
    std::this_thread::sleep_for(std::chrono::nanoseconds(calculateCost(particles.size(), move_part_ns)));

    particles.dumpParticles();
  }

  return 0;
}
