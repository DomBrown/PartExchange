#include "InputDeck.hpp"
#include "vt/transport.h"

int InputDeck::load(const char* name) {
  try {
    input_deck = YAML::LoadFile(name);
    
    nsteps = input_deck["Timesteps"].as<int>();
    nparticles = input_deck["Particle Count"].as<int>();
    ave_crossings = input_deck["Average Crossings"].as<double>();
    base_seed = input_deck["Crossing RNG Seed"].as<int>();
    rng_seed = base_seed + vt::theContext()->getNode();
    move_part_ns = input_deck["Move Particle Nanoseconds"].as<int>();
    migration_chance = input_deck["Migration Chance"].as<int>();
    
    dist_stdev = input_deck["Particle Distribution"]["Standard Deviation"].as<double>();
    overdecompose = input_deck["Overdecompose"].as<int>();
    
    if(vt::theContext()->getNumNodes()*overdecompose == 1) {
      migration_chance = 0;
      std::cout << "Running with only 1 rank: Forcing migration chance = 0!" << std::endl;
    }
    

    return 0;
  } catch(...) {
    fmt::print("Exception while loading input deck!\n");
    return -1;
  }
}
