#ifndef PARTICLE_HPP
#define PARTICLE_HPP

struct Particle {
  const int id;
  int num_moves;
  
  // Pad to 96 bytes so that sizes match
  char dummy_data[88];

  Particle(const int id_);
  Particle(const int id_, const int num_moves_);
};

#endif
