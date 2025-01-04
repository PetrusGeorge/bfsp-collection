#ifndef INSTANCE_H
#define INSTANCE_H

#include <vector>
#include <filesystem>

class Instance {
  public:
    Instance(const std::filesystem::path &path);

  private:
    size_t m_num_jobs = 0;
    size_t m_num_machines = 0;
    std::vector<std::vector<double>> matrix;
};

#endif
