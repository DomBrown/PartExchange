#include <vt/transport.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <string>

#include "yaml-cpp/yaml.h"

#include "ParticleContainer.hpp"
#include "ParticleMover.hpp"
#include "OutputWriter.hpp"

typedef vt::objgroup::proxy::Proxy<ParticleMover> PMProxyType;

static double total_time, start = 0.0;

// "Normally" distribute  particles over ranks. Return a vector containing counts for
// each rank we are using
std::vector<int> distributeParticles(const int nparticles, const int nranks, const double stdev, const int rng_seed) {

  std::vector<int> rank_counts;
  rank_counts.resize(nranks);

  std::mt19937 pseudorandom_generator(rng_seed);

  int maxrank = nranks - 1;

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
  }

  return rank_counts;
}

void executeStep(int step, int num_steps, PMProxyType& proxy) {
  auto me = vt::theContext()->getNode();

  ParticleMover* moverPtr = proxy[me].get();
	
  moverPtr->setNumMoves();
  vt::theCollective()->barrierThen([me, step]() {
    if(me == 0)
      fmt::print("Starting step {}\n", step);
  });

  // This will allow us to detect termination.
  auto epoch = vt::theTerm()->makeEpochCollective();

  vt::theTerm()->addAction(epoch, [step, num_steps, &proxy, moverPtr, me]{
    //fmt::print("{}: step={} finished\n", me, step);
    //fmt::print("Rank {} has {} particles\n", me, moverPtr->size());
    total_time += (vt::timing::Timing::getCurrentTime() - start);
    
    if (step+1 < num_steps) {
      executeStep(step+1, num_steps, proxy);
    } else {
      fmt::print("Node {} Final Count: {}\n", me, moverPtr->size());
      if(me == 0) {
        fmt::print("Total Time: {:.5f}\n", total_time);
      }
    }
  });

  // Start the work with a NullMsg to self
  auto msg = vt::makeSharedMessage<NullMsg>();
  vt::envelopeSetEpoch(msg->env, epoch);

  start = vt::timing::Timing::getCurrentTime();
  proxy[me].send<NullMsg, &ParticleMover::moveHandler>(msg);
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
  
  // Make sure each rank seed is different or else we have problems
  const int rng_seed = input_deck["Crossing RNG Seed"].as<int>() + rank;
  const int move_part_ns = input_deck["Move Particle Nanoseconds"].as<int>();
  int migration_chance;
  const double dist_stdev = input_deck["Particle Distribution"]["Standard Deviation"].as<double>();

  if(nranks > 1) {
    migration_chance = input_deck["Migration Chance"].as<int>();
  } else {
    migration_chance = 0;
    std::cout << "Running with only 1 rank: Forcing migration chance = 0!" << std::endl;
  }

  // Using the same seed removes the need for a bcast as everyone will get the same distro
  std::vector<int> rank_counts = distributeParticles(nparticles, nranks, dist_stdev, (rng_seed - rank));
  fmt::print("Rank {} has {} particles\n", rank, rank_counts[rank]);

  int my_start = 0;
  const int my_nparticles = rank_counts[rank];
  for(int i = 0; i < rank; i++)
    my_start += rank_counts[i];

  auto proxy = vt::theObjGroup()->makeCollective<ParticleMover>(my_nparticles, my_start, move_part_ns, ave_crossings, migration_chance, rng_seed);
 
  vt::theCollective()->barrierThen([]() {
    if(vt::theContext()->getNode() == 0)
      fmt::print("Initialised!\n");
  });
  executeStep(0, nsteps, proxy);

  while (!::vt::rt->isTerminated()) {
    vt::runScheduler();
  }
 
  vt::CollectiveOps::finalize();

  return 0;
}
