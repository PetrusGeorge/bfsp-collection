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
    size_t np() const { return m_np; }
    double mutation_factor() const { return m_mutation_factor; }
    double crossover_rate() const { return m_crossover_rate; }

    int min_ds() const { return m_min_ds; }
    int max_ds() const { return m_max_ds; }
    int min_ps() const { return m_min_ps; }
    int max_ps() const { return m_max_ps; }
    double min_tau() const { return m_min_tau; }
    double max_tau() const { return m_max_tau; }
    double min_jP() const { return m_min_jP; }
    double max_jP() const { return m_max_jP; }

private:
    std::string           m_instance_path;
    bool                  m_verbose = false;
    std::optional<size_t> m_seed;
    size_t                m_ro = 30;
    size_t                m_np = 10;
    double                m_mutation_factor = 0.5;
    double                m_crossover_rate = 0.9;

    int    m_min_ds{4};
    int    m_max_ds{8};
    int    m_min_ps{1};
    int    m_max_ps{4};
    double m_min_tau{0.1};
    double m_max_tau{1.0};
    double m_min_jP{0.0};
    double m_max_jP{1.0};
};

#endif
