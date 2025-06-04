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
    bool benchmark() const { return m_benchmark; }
    std::optional<size_t> tl() const { return m_tl; }
    size_t ro() const { return m_ro; }
    size_t ps() const { return m_ps; }
    double pmu() const { return m_pmu; }
    double pc() const { return m_pc; }
    double pls() const { return m_pls; }
    double theta() const { return m_theta; }
    size_t it() const { return m_it; }

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    std::optional<size_t> m_seed;
    std::optional<size_t> m_tl;
    bool m_benchmark = false;
    size_t m_ro = 100;
    size_t m_ps = 20;
    double m_pmu = 0.9;
    double m_pc = 0.1;
    double m_pls = 0.2;
    double m_theta = 0.75;
    size_t m_it = 1;
};

#endif
