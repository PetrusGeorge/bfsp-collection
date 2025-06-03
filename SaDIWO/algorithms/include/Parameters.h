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

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    std::optional<size_t> m_seed;
    size_t m_ro = 30;
};

#endif
