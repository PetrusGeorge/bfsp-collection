#ifndef CLI_H
#define CLI_H

#include <cstddef>
#include <optional>
#include <string>

class Parameters {
  public:
    Parameters(int argc, char **argv);

    const std::string &instance_path() const { return m_instance_path; }
    bool verbose() const { return m_verbose; }
    bool benchmark() const { return m_benchmark; }
    std::optional<size_t> seed() const { return m_seed; }
    double alpha() const { return m_alpha; }
    double beta() const { return m_beta; }
    size_t d() const { return m_d; }
    size_t k() const { return m_k; }

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    bool m_benchmark = false;
    std::optional<size_t> m_seed;
    double m_alpha = 0.5;
    double m_beta = 1;
    size_t m_d = 6;
    size_t m_k = 30;
};

#endif
