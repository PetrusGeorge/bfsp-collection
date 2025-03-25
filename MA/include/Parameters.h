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
    size_t ps() const { return m_ps; }
    size_t gamma() const { return m_gamma; }
    size_t lambda() const { return m_lambda; }
    double pc() const { return m_pc; }
    double pm() const { return m_pm; }
    

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    std::optional<size_t> m_seed;
    size_t m_ps = 10;
    size_t m_gamma = 20;
    size_t m_lambda = 20;
    double m_pc = 0.2;
    double m_pm = 0.8;
};

#endif
