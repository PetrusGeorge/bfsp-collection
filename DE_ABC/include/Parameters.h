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
    size_t ro() const { return m_ro; }
    size_t ps() const { return m_ps; }
    double pmu() const { return m_pmu; }
    double pc() const { return m_pc; }
    double pls() const { return m_pls; }

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    std::optional<size_t> m_seed;
    size_t m_ro = 5;
    size_t m_ps = 20;
    double m_pmu = 0.9;
    double m_pc = 0.1;
    double m_pls = 0.2;
};

#endif
