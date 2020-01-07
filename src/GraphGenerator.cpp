#include "GraphGenerator.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <random>
#include <algorithm>
#include <cstdlib>

GraphGenerator::GraphGenerator(int num_nodes_, double ave_degree_, int seed_) :
  num_nodes(num_nodes_), ave_degree(ave_degree_) {
  
  matrix.resize(num_nodes_);
  for(int i = 0; i < num_nodes_; i++)
    matrix[i].resize(num_nodes_);

  for(int i = 0; i < num_nodes_; i++) {
    for(int j = 0; j < num_nodes_; j++) {
      if(i < j) {
        positions.emplace_back(i, j);
      }
    }
  }

  // Randomise the order in advance
  std::mt19937 eng(seed_);
  std::shuffle(std::begin(positions), std::end(positions), eng);

  generateGraph();
}

void GraphGenerator::generateGraph() {
  // Connect even nodes to odds
  for(int i = 0; i < num_nodes; i+=2) {
    if(averageDegree() >= ave_degree)
      break;
    if((i + 1) < num_nodes)
      addEdge(i, i+1);
  }

  // Make chain by connecting odds to evens
  for(int i = 1; i < num_nodes; i+=2) {
    if(averageDegree() >= ave_degree)
      break;
    if((i + 1) < num_nodes)
      addEdge(i, i+1);
  }

  // Close the loop
  if(averageDegree() < ave_degree)
    addEdge(0, num_nodes-1);

  // Now we add connections until desired average is reached
  // Or we run out of possible edges to add
  while(positions.size()) {
    if(averageDegree() >= ave_degree)
      break;

    auto& elem = positions[0];
    addEdge(elem.first, elem.second);
  }
}

void GraphGenerator::addEdge(const int i, const int j) {
  if(matrix[i][j] == 0 && matrix[j][i] == 0) {
    matrix[i][j] = 1;
    matrix[j][i] = 1;
    num_edges++;
    positions.erase(std::remove(std::begin(positions), std::end(positions), std::make_pair(i, j)));
    edges.emplace_back(i, j);
  }
}

void GraphGenerator::printMatrix() {
  std::cout << "\t";
  for(int i = 0; i < num_nodes; i++)
    std::cout << i << "\t";
  std::cout << std::endl;

  for(int i = 0; i < num_nodes; i++) {
    std::cout << i << "\t";
    for(int j = 0; j < num_nodes; j++) {
      std::cout << matrix[i][j] << "\t";
    }
    std::cout << std::endl;
  }
}

const std::vector<std::pair<int, int>>& GraphGenerator::getEdgeVector() {
  return edges;
}

const std::vector<int> GraphGenerator::getNodeNeighbours(const int node) {
  std::vector<int> ret;
  for(int i = 0; i < matrix[node].size(); i++) {
    if(matrix[node][i] == 1)
      ret.push_back(i);
  }

  return ret;
}

void GraphGenerator::printPositions() {
  for(auto& elem : positions)
    std::cout << "(" << elem.first << ", " << elem.second << ")" << std::endl;
}

double GraphGenerator::averageDegree() {
  if(num_nodes == 0)
    return 0;
  else return (2 * num_edges) / num_nodes;
}
