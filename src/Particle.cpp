#include "Particle.hpp"

#include <iostream>

Particle::Particle() {
  id = -1;
  num_moves = -1;
}

Particle::Particle(const int id_) : id(id_), num_moves(0) {}

Particle::Particle(const int id_, const int num_moves_) : id(id_), num_moves(num_moves_) {}

Particle::Particle(const Particle& in) {
  id = in.id;
  num_moves = in.num_moves;
}

Particle& Particle::operator=(const Particle& in) {
  id = in.id;
  num_moves = in.num_moves;
  return *this;
} 
