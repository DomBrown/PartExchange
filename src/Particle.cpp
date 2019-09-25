#include "Particle.hpp"

#include <iostream>

Particle::Particle(const unsigned int id_) : id(id_), num_moves(0) {}

Particle::Particle(const unsigned int id_, const unsigned int num_moves_) : id(id_), num_moves(num_moves_) {}
