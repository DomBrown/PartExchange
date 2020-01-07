#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <random>
#include <algorithm>
#include <cstdlib>

class GraphGenerator {

  public:
    GraphGenerator() = delete;

    GraphGenerator(int num_nodes_, double ave_degree_, int seed_);

    void generateGraph();

    void addEdge(const int i, const int j);

    void printMatrix();

    const std::vector<std::pair<int, int>>& getEdgeVector();

    const std::vector<int> getNodeNeighbours(const int node);

    void printPositions();

    double averageDegree();

  private:
    int num_nodes;
    double ave_degree;
    std::vector<std::vector<int>> matrix;
    std::vector<std::pair<int, int>> positions;
    std::vector<std::pair<int, int>> edges;
    int num_edges = 0;
};
