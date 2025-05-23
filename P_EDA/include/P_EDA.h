#ifndef P_EDA_H
#define P_EDA_H
#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"

using SizeTMatrix = std::vector<std::vector<size_t>>;

// P_EDA algorithm - DOI: 10.1080/0305215X.2017.1353090

class P_EDA { // NOLINT
  public:
    P_EDA(Instance &instance, Parameters &params, size_t ps = 50, double lambda = 0.3);
    Solution solve();

  private:
    // functions related to the initialization of the population
    static inline bool is_similar(const std::vector<size_t> &a, const std::vector<size_t> &b,
                                  size_t similarity_threshold);
    void generate_initial_population();
    void generate_random_individuals();
    void modified_linear_rank_selection();

    bool mrls(Solution &s, std::vector<size_t> &ref, Instance &instance);

    // functions related to the probabilistic model
    Solution probabilistic_model(const SizeTMatrix &p, const std::vector<SizeTMatrix> &t);
    std::vector<std::vector<size_t>> get_p();
    std::vector<SizeTMatrix> get_t();
    std::vector<double> get_probability_vector(const std::vector<size_t> &sequence,
                                               const std::vector<size_t> &unasigned_jobs, const SizeTMatrix &p,
                                               const std::vector<SizeTMatrix> &t);

    // Path relink
    Solution path_relink_swap(const Solution &alpha, const Solution &beta);
    inline void mutation(Solution &individual);

    // misc
    std::vector<size_t> fisher_yates_shuffle();
    std::pair<bool, size_t> in_population_and_max_makespan(std::vector<size_t> &sequence);
    double get_diversity();
    void population_regen();

    void print_pc() const;

    Instance &m_instance;
    Parameters &m_params;

    // the values of lambda and population size are said to be the best ones according to Shao

    double m_lambda = 0.3;      // diversity threshold (differs from lambda used in pf-neh)
    size_t m_ps = 50;           // population size
    std::vector<Solution> m_pc; // population vector
};

#endif
