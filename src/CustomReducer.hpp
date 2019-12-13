#ifndef CUSTOM_REDUCER_HPP
#define CUSTOM_REDUCER_HPP
#include <vt/transport.h>
#include <vector>
#include <utility>

struct CustomPayload {
  CustomPayload() = default;
  CustomPayload(int in_node, int in_count) {
    vec.emplace_back(in_node, in_count);
  }

  friend CustomPayload operator+(CustomPayload& in1, CustomPayload const& in2) {

    for(auto& elem : in2.vec) {
      in1.vec.push_back(elem);
    }

    return in1;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | vec;
  }

  std::vector<std::pair<int, int>> vec;
};

// Msg to carry the custom payload we reduce into the result
struct CustomPayloadMsg : vt::collective::ReduceTMsg<CustomPayload> {
  CustomPayloadMsg() = default;

  CustomPayloadMsg(int in_node, int in_count) : vt::collective::ReduceTMsg<CustomPayload>() {
    auto& vec = getVal().vec;
    vec.push_back(std::make_pair(in_node, in_count));
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    ReduceTMsg<CustomPayload>::invokeSerialize(s);
  }
};

struct PrintReduceResult {
  PrintReduceResult() = default;
  ~PrintReduceResult() = default;

  void operator() (CustomPayloadMsg* msg) {
    const auto& vec = msg->getConstVal().vec;
    int total = 0;
        
    for(auto& elem : vec) {
      total += elem.second;
      fmt::print("Tile {} has {} particles\n", elem.first, elem.second);
    }   
    fmt::print("Total Particles: {}\n", total);
  }
};

#endif
