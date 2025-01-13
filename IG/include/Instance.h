#ifndef INSTANCE_H
#define INSTANCE_H

#include <filesystem>
#include <vector>

class Instance {
  public:
    // The constructor of this class can throw in case it can't read the isntance properly
    Instance(const std::filesystem::path &path);

    size_t num_jobs() const { return m_num_jobs; }
    size_t num_machines() const { return m_num_machines; }

    // Main instance matrix
    long p(size_t i, size_t j) const { return m_matrix[i][j]; }
    // Reverse matrix
    long rp(size_t i, size_t j) const { return m_matrix[i][m_num_machines - j - 1]; }

  private:
    size_t m_num_jobs = 0;
    size_t m_num_machines = 0;
    std::vector<std::vector<long>> m_matrix;
};

#endif
