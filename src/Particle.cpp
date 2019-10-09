#include "Particle.hpp"

#include <iostream>
#include <cstring>

Particle::Particle() {
  id = -1;
  num_moves = -1;
}

Particle::Particle(const int id_) : id(id_), num_moves(0), dead(0) {}

Particle::Particle(const int id_, const int num_moves_) : id(id_), num_moves(num_moves_), dead(0) {}

Particle::Particle(const Particle& in) {
  id = in.id;
  num_moves = in.num_moves;
  dead = in.dead;

  std::memcpy(dummy_data, in.dummy_data, 84);
}

Particle& Particle::operator=(const Particle& in) {
  id = in.id;
  num_moves = in.num_moves;
  dead = in.dead;
  
  std::memcpy(dummy_data, in.dummy_data, 84);
  return *this;
} 
