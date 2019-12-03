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

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | id | num_moves | dead;

    for(int i = 0; i < 84; i++) {
      s | dummy_data[i];
    }
  }
};

/*struct ParticleMsg : vt::Message {
  ParticleMsg() = default;

  // Add a serialiser that will serialise the particle vector
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | particles;
  }

  public:
    std::vector<Particle> particles;
};*/

#endif
