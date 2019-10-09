#ifndef PARTICLE_HPP
#define PARTICLE_HPP

struct Particle {
  int id;
  int num_moves;
  int dead;
  
  // Pad to 96 bytes so that sizes match
  char dummy_data[84];

  Particle();

  Particle(const int id_);
  Particle(const int id_, const int num_moves_);

  Particle(const Particle& in);

  Particle& operator=(const Particle & in);
};

#endif
