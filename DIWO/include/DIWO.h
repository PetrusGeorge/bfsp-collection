#ifndef DIWO_H
#define DIWO_H

#include "Instance.h"
#include "RNG.h"
#include "Solution.h"
#include <random>

struct DIWOParams {
    size_t p_max{10};
    double pls{0.15};
    size_t s_min{0};
    size_t s_max{7};
    size_t sigma_min{0};
    size_t sigma_max{5};
};

class Population {
  public:
    std::vector<Solution> solutions;
    std::vector<int> seeds;
    size_t best_solution_idx;
    size_t worst_solution_idx;

    void add_solution(Solution solution);
    bool has_solution(const Solution &solution) const;
    void calculate_seeds(const DIWOParams &params);
};

class DIWO {
  public:
    DIWO(Instance instance, DIWOParams params) : m_instance(std::move(instance)), m_params(params) {}

    Solution solve();

  private:
    Instance m_instance;
    DIWOParams m_params;
    std::mt19937 &m_rng = RNG::instance().gen();

    Population spatial_dispersal(const Population &pop);
    size_t get_solution_d(double deviation);

    void local_search(Population &pop);
    Population competitive_exclusion(Population pop, Population new_pop) const;

    Population population_init();
};

#endif
