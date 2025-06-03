#ifndef CLI_H
#define CLI_H

#include <cstddef>
#include <optional>
#include <string>

class Parameters {
  public:
    Parameters(int argc, char **argv);

    const std::string &instance_path() const { return m_instance_path; }
    std::optional<size_t> seed() const { return m_seed; }
    double alpha() const { return m_alpha; }
    double pls() const { return m_pls; }
    size_t ps() const { return m_ps; }
    size_t t() const { return m_T; }
    bool benchmark() const { return m_benchmark; }
    size_t ro() const { return m_ro; }

  private:
    std::string m_instance_path;
    std::optional<size_t> m_seed;
    bool m_benchmark = false;
    size_t m_ro = 30;
    double m_alpha = 0.6;
    double m_pls = 0.6; // The heuristic presents its best performance with φ = 0.75
    size_t m_ps = 20;
    size_t m_T = 5;
};

#endif
