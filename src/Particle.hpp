#ifndef PARTICLE_HPP
#define PARTICLE_HPP
#include <vt/transport.h>
#include <vector>

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

struct ParticleMsg : vt::Message {
  ParticleMsg() = default;

  // Add a serialiser that will serialise the particle vector
  template <typename SerializerType>
  void serialize(SerializerType& s) {
    s | particles;
  }

  public:
    std::vector<Particle> particles;
};

#endif
