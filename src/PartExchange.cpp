#include <vt/transport.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <string>

#include "yaml-cpp/yaml.h"

#include "InputDeck.hpp"
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

  start = vt::timing::Timing::getCurrentTime();
  // Start the work
  // TODO Try an implementation where each node starts its own
  // tasks and see if that performs better
  if(me == 0) {
    auto msg = vt::makeSharedMessage<ParticleMover::NullMsg>();
    vt::envelopeSetEpoch(msg->env, epoch);
    proxy.broadcast<ParticleMover::NullMsg, &ParticleMover::moveHandler>(msg);
  }

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

  InputDeck deck;

  if(argc < 2) {
    std::cout << "Provide an input YAML file!" << std::endl;
    vt::CollectiveOps::finalize();

    return 0;
  } else {
    int ret = deck.load(argv[1]);
    if(ret == -1) {
      vt::CollectiveOps::finalize();
      return 0;
    }
  }

  // Using the same seed removes the need for a bcast as everyone will get the same distro
  std::vector<int> rank_counts = distributeParticles(deck.nparticles, nranks, deck.dist_stdev, deck.base_seed);
  fmt::print("Rank {} has {} particles\n", rank, rank_counts[rank]);

  int my_start = 0;
  const int my_nparticles = rank_counts[rank];
  for(int i = 0; i < rank; i++)
    my_start += rank_counts[i];

  using BaseIndexType = typename IndexType::DenseIndexType;
  auto const& range = IndexType(static_cast<BaseIndexType>(nranks*deck.overdecompose));

  auto proxy = vt::theCollection()->constructCollective<ParticleMover>(
    range, [&deck, rank, nranks, &rank_counts, my_start] (IndexType idx) {
      // Each tile needs a unique seed
      int tile_seed = deck.base_seed + idx.x();
      
      return std::make_unique<ParticleMover>(
        rank_counts[rank]/deck.overdecompose,
        my_start,
        deck.move_part_ns,
        deck.ave_crossings,
        deck.migration_chance,
        tile_seed,
        nranks*deck.overdecompose
      );
    }
  );
 
  vt::theCollective()->barrierThen([]() {
    if(vt::theContext()->getNode() == 0)
      fmt::print("Initialised!\n");
  });
  
  initStep(0, deck.nsteps, proxy);

  while (!::vt::rt->isTerminated()) {
    vt::runScheduler();
  }
 
  vt::CollectiveOps::finalize();

  return 0;
}
