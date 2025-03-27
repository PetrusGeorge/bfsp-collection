#ifndef P_EDA_H
#define P_EDA_H
#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"

using SizeTMatrix = std::vector<std::vector<size_t>>;

class P_EDA { // NOLINT
  public:
    P_EDA(Instance &instance, Parameters &params, size_t ps = 50, double lambda = 0.3);
    Solution solve();

  private:
    size_t calculate_similarity(const std::vector<size_t> &random_sequence, const Solution &individual);
    void generate_initial_population();
    void generate_random_individuals();
    void modified_linear_rank_selection();

    std::vector<size_t> probabilistic_model(const SizeTMatrix &p, const std::vector<SizeTMatrix> &t);
    std::vector<std::vector<size_t>> get_p();
    std::vector<SizeTMatrix> get_t();
    std::vector<double> get_probability_vector(const std::vector<size_t> &sequence,
                                               const std::vector<size_t> &candidate_jobs, const SizeTMatrix &p,
                                               const std::vector<SizeTMatrix> &t);

    Solution path_relink_swap(const Solution &alpha, const Solution &beta, size_t similarity_threshold);
    void mutation(Solution &individual);

    std::vector<size_t> fisher_yates_shuffle();
    std::pair<bool, size_t> in_population_and_max_makespan(std::vector<size_t> &sequence);
    double get_diversity();
    void population_regen();

    void print_pc() const;
    static void print_count(std::vector<std::vector<size_t>> &count);

    Instance &m_instance;
    Parameters &m_params;

    double m_lambda = 0.3;      // diversity threshold (differs from lambda used in pf-neh)
    size_t m_ps = 50;           // population size
    std::vector<Solution> m_pc; // population vector
};

#endif
