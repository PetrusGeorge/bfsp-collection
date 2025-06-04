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
    size_t ro() const { return m_ro; }

    double pls() const { return m_pls; }
    size_t p_max() const { return m_p_max; }
    size_t s_min() const { return m_s_min; }
    size_t s_max() const { return m_s_max; }
    size_t sigma_min() const { return m_sigma_min; }
    size_t sigma_max() const { return m_sigma_max; }

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    bool m_benchmark = false;
    std::optional<size_t> m_seed;
    size_t m_ro = 30;

    double m_pls = 0.15;
    size_t m_p_max = 3;
    size_t m_s_min = 0;
    size_t m_s_max = 2;
    size_t m_sigma_min = 3;
    size_t m_sigma_max = 6;
};

#endif
