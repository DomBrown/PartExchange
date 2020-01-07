#ifndef INPUT_DECK_HPP
#define INPUT_DECK_HPP
#include "yaml-cpp/yaml.h"
#include "fmt/format.h"

struct InputDeck {
  public:
    InputDeck() = default;

    int load(const char* name);

    ~InputDeck() = default;

    YAML::Node input_deck;
    int nsteps, nparticles, base_seed, rng_seed, move_part_ns, migration_chance, overdecompose;;
    double ave_crossings, dist_stdev, ave_neighbours;
};
#endif
