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

using IndexType = vt::IdxType1D<std::size_t>;
using PMProxyType = vt::vrt::collection::CollectionProxy<ParticleMover, IndexType>;

static double total_time, start = 0.0;

// Forward declare these so we can use in term calls
void initStep(int step, int num_steps, PMProxyType& proxy);
void executeStep(int step, int num_steps, PMProxyType& proxy);

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

  // This will allow us to detect termination.
  auto epoch = vt::theTerm()->makeEpochCollective();

  vt::theTerm()->addAction(epoch, [step, num_steps, &proxy, me]{
    total_time += (vt::timing::Timing::getCurrentTime() - start);
    
    if (step+1 < num_steps) {
      initStep(step+1, num_steps, proxy);
    } else {
      if(me == 0) {
        fmt::print("Total Time: {:.5f}\n", total_time);

        // Gather up the particle counts to write out
        // Also sum to ensure none have been lost
        auto msg = vt::makeSharedMessage<ParticleMover::NullMsg>();
        proxy.broadcast<ParticleMover::NullMsg, &ParticleMover::printParticleCountsHandler>(msg);
      }
    }
  });

  auto msg = vt::makeSharedMessage<ParticleMover::NullMsg>();
  vt::envelopeSetEpoch(msg->env, epoch);

  start = vt::timing::Timing::getCurrentTime();
  proxy[me].send<ParticleMover::NullMsg, &ParticleMover::moveHandler>(msg);
  
  vt::theTerm()->finishedEpoch(epoch);
}

void initStep(int step, int num_steps, PMProxyType& proxy) {
  auto me = vt::theContext()->getNode();
  
  auto epoch = vt::theTerm()->makeEpochCollective();

  vt::theTerm()->addAction(epoch, [step, num_steps, &proxy, me]{
    if(me == 0)
      fmt::print("Starting step {}\n", step);
    
    executeStep(step, num_steps, proxy);
  });


  // Start the work
  // TODO Try an implementation where each node starts its own
  // tasks and see if that performs better
  if(me == 0) {
    auto msg = vt::makeSharedMessage<ParticleMover::NullMsg>();
    vt::envelopeSetEpoch(msg->env, epoch);
    proxy.broadcast<ParticleMover::NullMsg, &ParticleMover::setNumMovesHandler>(msg);
  }

  vt::theTerm()->finishedEpoch(epoch);
}


int main(int argc, char** argv) {

  vt::CollectiveOps::initialize(argc, argv);

  int rank = vt::theContext()->getNode();
  int nranks = vt::theContext()->getNumNodes();

  YAML::Node input_deck;

  if(argc < 2) {
    std::cout << "Provide an input YAML file!" << std::endl;
    vt::CollectiveOps::finalize();

    return 0;
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
  const int overdecompose = input_deck["Overdecompose"].as<int>();

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

  using BaseIndexType = typename IndexType::DenseIndexType;
  auto const& range = IndexType(static_cast<BaseIndexType>(nranks*overdecompose));

  auto proxy = vt::theCollection()->constructCollective<ParticleMover>(
    range, [rank, overdecompose, nranks, &rank_counts, my_start, move_part_ns, ave_crossings, migration_chance, rng_seed] (IndexType idx) {
      return std::make_unique<ParticleMover>(rank_counts[rank]/overdecompose, my_start, move_part_ns, ave_crossings, migration_chance, rng_seed, nranks*overdecompose);
    }
  );
 
  vt::theCollective()->barrierThen([]() {
    if(vt::theContext()->getNode() == 0)
      fmt::print("Initialised!\n");
  });
  
  initStep(0, nsteps, proxy);

  while (!::vt::rt->isTerminated()) {
    vt::runScheduler();
  }
 
  vt::CollectiveOps::finalize();

  return 0;
}
