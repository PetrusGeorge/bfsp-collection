#ifndef INSTANCE_H
#define INSTANCE_H

#include <filesystem>
#include <vector>

struct Instance {
  public:
    Instance(const std::filesystem::path &path);

    size_t num_jobs() const { return m_num_jobs; }
    size_t num_machines() const { return m_num_machines; }
    const std::vector<size_t> &processing_times_sum() const { return m_processing_times_sum; }

    long p(size_t i, size_t j) const { return m_matrix[i][j]; }
    // Reverse matrix
    long rp(size_t i, size_t j) const { return m_matrix[i][m_num_machines - j - 1]; }

    Instance create_reverse_instance();

  private:
    size_t m_num_jobs = 0;
    size_t m_num_machines = 0;
    std::vector<std::vector<long>> m_matrix;
    std::vector<size_t> m_processing_times_sum;

    void calculate_processing_times_sum();
};

#endif
