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
    size_t ms() const { return m_ms; }
    double pcr() const { return m_pcr; }
    double par() const { return m_par; }
    

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    std::optional<size_t> m_seed;
    size_t m_ro = 30;
    size_t m_ms = 5;
    double m_pcr = 0.95;
    double m_par = 0.99;
};

#endif
