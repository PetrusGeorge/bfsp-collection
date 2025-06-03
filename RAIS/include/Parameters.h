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
  std::optional<size_t> time_limit() const { return m_time_limit; }
  size_t ro() const { return m_ro; }
  size_t d_threshold() const { return m_d_threshold; }
  size_t Gt() const { return m_Gt; }
  size_t nc() const { return m_nc; }
  double alpha() const { return m_alpha; }

private:
  std::string m_instance_path;
  bool m_verbose = false;
  std::optional<size_t> m_time_limit;
  std::optional<size_t> m_seed;
  bool m_benchmark = false;
  size_t m_ro = 100;
  size_t m_d_threshold = 5;
  size_t m_Gt = 4000;
  size_t m_nc = 6;
  double m_alpha = 0.97;
};

#endif
