#include <mpi.h>
#include <vt/transport.h>

#include <iostream>
#include <chrono>
#include <thread>

#include "ParticleContainer.hpp"

int calculateCost(const int num_parts, const double cost_per_part) {
  return num_parts * cost_per_part;
}

int main(int argc, char** argv) {
  
  unsigned int global_start_id = 0;
  const double cost_per_part = 5;  // currently in milliseconds
  const int nsteps = 2;

  ParticleContainer particles(global_start_id);

  particles.reserve(100);

  for(int i = 0; i < 100; i++)
    particles.addParticle();

  for(int step = 0; step < nsteps; step++) {
    std::cout << "Step" << std::endl;
    
    particles.setNumMoves();

    std::this_thread::sleep_for(std::chrono::milliseconds(calculateCost(particles.size(), cost_per_part)));

    particles.dumpParticles();
  }

  return 0;
}
