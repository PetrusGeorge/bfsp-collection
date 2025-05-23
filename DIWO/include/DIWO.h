#ifndef DIWO_H
#define DIWO_H

#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"
#include <random>
#include <utility>

class Population {
  public:
    std::vector<Solution> solutions;
    std::vector<size_t> seeds;
    size_t best_solution_idx;
    size_t worst_solution_idx;

    void add_solution(Solution solution);
    bool has_solution(const Solution &solution) const;
    void calculate_seeds(size_t s_min, size_t s_max);
};

class DIWO {
  public:
    DIWO(Instance instance, Parameters params) : m_instance(std::move(instance)), m_params(std::move(params)) {}

    Solution solve();

  private:
    Instance m_instance;
    Parameters m_params;
    std::mt19937 &m_rng = RNG::instance().gen();

    Population spatial_dispersal(const Population &pop);
    size_t get_solution_d(double deviation);

    void local_search(Population &pop);
    Population competitive_exclusion(Population pop, Population new_pop) const;

    Population population_init();
};

#endif
