#ifndef PARTICLE_H
#define PARTICLE_H

struct Particle {
  const unsigned int id;
  unsigned int num_moves;
  
  // Pad to 96 bytes so that sizes match
  char dummy_data[88];

  Particle(const unsigned int id_);
  Particle(const unsigned int id_, const unsigned int num_moves_);
};

#endif
