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
    size_t np() const { return m_np; }
    size_t delta() const { return m_delta; }
    size_t x() const { return m_x; }
    double cr() const { return m_cr; }
    double beta() const { return m_beta; }
    double f() const { return m_f; }

    size_t ds_min() const { return m_ds_min; }
    size_t ds_max() const { return m_ds_max; }
    size_t ps_min() const { return m_ps_min; }
    size_t ps_max() const { return m_ps_max; }
    double tau_min() const { return m_tau_min; }
    double tau_max() const { return m_tau_max; }
    double jp_min() const { return m_jp_min; }
    double jp_max() const { return m_jp_max; }

  private:
    std::string m_instance_path;
    bool m_verbose = false;
    std::optional<size_t> m_seed;
    bool m_benchmark = false;
    std::optional<size_t> m_time_limit;
    size_t m_ro = 100;
    size_t m_np = 10;
    size_t m_delta = 20;
    size_t m_x = 15;
    double m_cr = 0.1;
    double m_beta = 5e-4;
    double m_f = 0.1;
    size_t m_ds_min = 4;
    size_t m_ds_max = 8;
    size_t m_ps_min = 1;
    size_t m_ps_max = 4;
    double m_tau_min = 0.1;
    double m_tau_max = 1.0;
    double m_jp_min = 0;
    double m_jp_max = 1;
};

#endif
