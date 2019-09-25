#include <mpi.h>
#include <vt/transport.h>

#include <iostream>
#include <vector>

#include "Particle.hpp"

int main(int argc, char** argv) {
  
  unsigned int global_id = 0;

  std::vector<Particle> particles;

  particles.emplace_back(global_id++);
  particles.emplace_back(global_id++);
  particles.emplace_back(global_id++);

  for(int i = 0; i < particles.size(); i++)
    std::cout << particles[i].id << std::endl;

  return 0;
}
