#ifndef INSTANCE_H
#define INSTANCE_H

#include <filesystem>
#include <vector>

struct Instance {
  public:
    Instance(const std::filesystem::path &path);

    size_t num_jobs() const { return m_num_jobs; }
    size_t num_machines() const { return m_num_machines; }
    std::vector<std::vector<size_t>> matrix() const { return m_matrix; }

  private:
    size_t m_num_jobs = 0;
    size_t m_num_machines = 0;
    std::vector<std::vector<size_t>> m_matrix;
};

#endif
