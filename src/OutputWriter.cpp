#include "OutputWriter.hpp"

#include <algorithm>
#include <fmt/core.h>
#include "yaml-cpp/yaml.h"

OutputWriter::OutputWriter(const std::string& fname_) {
  if(fname_.empty()) {
    outfile.open("output.out.yaml");
  } else {
    outfile.open(fname_);
  }
}

OutputWriter::~OutputWriter() {
  outfile.close();
}

void OutputWriter::writeStatistics(const std::string& name, const std::vector<double>& data) {
  double average, total = 0.; 
  double max = -1e99;
  double min = 1e99;

  for(auto& d : data) {
    total += d;
    min = std::min(min, d); 
    max = std::max(max, d); 
  }

  average = total / data.size();

  double sum_devs = 0.; 
  for(auto& d : data) {
    sum_devs += pow((d - average), 2); 
  }

  sum_devs /= data.size();
  double stdev = sqrt(sum_devs);

  YAML::Node parent;
  YAML::Node dataNode;

  dataNode["Min"] = fmt::format("{:.5f}", min);
  dataNode["Average"] = fmt::format("{:.5f}", average);
  dataNode["Max"] = fmt::format("{:.5f}", max);
  dataNode["StDev"] = fmt::format("{:.5f}", stdev);

  parent[name] = dataNode;

  outfile << parent;
  outfile << std::endl << std::endl;
}
