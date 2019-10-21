#ifndef OUTPUT_WRITER_HPP
#define OUTPUT_WRITER_HPP

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class OutputWriter {
  public:
    OutputWriter() = delete;
    OutputWriter(const std::string& fname_);

    void writeStatistics(const std::string& name, const std::vector<double>& data);

    ~OutputWriter();
  
  private:
    std::ofstream outfile;
};

#endif
