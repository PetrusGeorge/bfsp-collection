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
    std::optional<size_t> tl() const { return m_tl; }
    size_t ro() const { return m_ro; }
    double tP() const { return m_tP; }
    size_t d() const { return m_dS; }

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    bool m_benchmark = false;
    std::optional<size_t> m_seed;
    std::optional<size_t> m_tl;
    size_t m_ro = 10;
    double m_tP = 0.5;
    size_t m_dS = 8;
};

#endif