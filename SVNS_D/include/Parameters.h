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
    std::optional<size_t> seed() const { return m_seed; }
    double alpha() const { return m_alpha; }
    double beta() const { return m_beta; }
    size_t d() const { return m_d; }
    size_t ro() const { return m_ro; }
    bool benchmark() const { return m_benchmark; }

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    std::optional<size_t> m_seed;
    double m_alpha = 0.0;
    double m_beta = 0.0;
    size_t m_d = 0;
    size_t m_ro = 30;
    bool m_benchmark = false;
};

#endif
