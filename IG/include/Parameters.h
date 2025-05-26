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
    bool becnhmark() const { return m_benchmark; }
    std::optional<size_t> seed() const { return m_seed; }
    std::optional<size_t> tl() const { return m_tl; }
    size_t ro() const { return m_ro; }
    double alpha() const { return m_alpha; }
    size_t d() const { return m_d; }

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    bool m_benchmark = false;
    std::optional<size_t> m_seed;
    std::optional<size_t> m_tl;
    size_t m_ro = 30;
    double m_alpha = 0.6;
    size_t m_d = 6;
};

#endif
