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
    size_t n_iter() const { return m_n_iter; }
    double final_temperature() const { return m_final_temperature; }
    double alpha() const { return m_alpha; }

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    bool m_benchmark = false;
    std::optional<size_t> m_seed;
    std::optional<size_t> m_tl;
    size_t m_ro = 100;
    size_t m_n_iter = 1800000;
    double m_final_temperature = 1;
    double m_alpha = 0.8;
};

#endif
