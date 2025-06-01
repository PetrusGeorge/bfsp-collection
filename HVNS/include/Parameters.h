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
    std::optional<size_t> time_limit() const { return m_tl; }
    size_t ro() const { return m_ro; }
    size_t n_iter() const { return m_n_iter; }

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    bool m_benchmark = false;
    std::optional<size_t> m_seed;
    std::optional<size_t> m_tl;
    size_t m_ro = 100;
    size_t m_n_iter = 1800000;
};

#endif
