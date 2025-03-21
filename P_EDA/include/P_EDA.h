#ifndef P_EDA_H
#define P_EDA_H
#include "Instance.h"
#include "Solution.h"

class P_EDA { // NOLINT
  public:
    P_EDA(Instance &instance);
    Solution solve();

  private:
    size_t calculate_similarity(const std::vector<size_t> &random_sequence, const Solution &individual);
    void generate_initial_population();
    void generate_random_individuals();
    void modified_linear_rank_selection();
    void print_pc();

    Instance &m_instance;

    double m_lambda = 0.3;      // diversity threshold (differs from lambda used in pf-neh)
    size_t m_ps = 50;           // population size
    std::vector<Solution> m_pc; // population vector
};

#endif
