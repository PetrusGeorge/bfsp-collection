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
    size_t ro() const { return m_ro; }
    size_t ps() const { return m_ps; }
    double z() const { return m_z; }
    double cr() const { return m_cr; }
    double pl() const { return m_pl; }

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    std::optional<size_t> m_seed;
    bool m_benchmark = false;
    size_t m_ro = 100;
    size_t m_ps = 20;
    double m_z = 0.2;
    double m_cr = 0.2;
    double m_pl = 0.2;
};

#endif
