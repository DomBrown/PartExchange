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

void executeStep(int step, int num_steps, ParticleContainer& particles) {
	particles.setNumMoves();
  vt::theCollective()->barrier();
  
  // This will allow us to detect termination.
  auto epoch = vt::theTerm()->makeEpochCollective();

  vt::theTerm()->addAction(epoch, [step, num_steps, &particles]{
    fmt::print("{}: step={} finished\n", vt::theContext()->getNode(), step);
    if (step+1 < num_steps) {
      executeStep(step+1, num_steps, particles);
    }
  });

  particles.moveKernel(0, particles.size());
  particles.dumpParticles();

  vt::theTerm()->finishedEpoch(epoch);
}

int main(int argc, char** argv) {

  vt::CollectiveOps::initialize(argc, argv);

  int rank = vt::theContext()->getNode();
  int nranks = vt::theContext()->getNumNodes();

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
    migration_chance = 0;
  } else {
    migration_chance = 0;
    std::cout << "Running with only 1 rank: Forcing migration chance = 0!" << std::endl;
  }

  // Hardcoded even particle distribution for ease of porting
  const int my_nparticles = nparticles / nranks;
  const int my_start = my_nparticles * rank;

  ParticleContainer particles(move_part_ns, my_start, ave_crossings, migration_chance, rng_seed);
  particles.reserve(my_nparticles);

  for(int i = 0; i < my_nparticles; i++) {
    particles.addParticle();
  }

  executeStep(0, nsteps, particles);

  // This drains the system of work before we finalise.
  while (!::vt::rt->isTerminated()) {
    vt::runScheduler();
  }
 
  //vt::theCollective()->barrier();
  vt::CollectiveOps::finalize();

  return 0;
}
