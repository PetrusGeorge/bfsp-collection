#ifndef SADIWO_H
#define SADIWO_H

#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"
#include <random>

class Population {
  public:
    std::vector<Solution> solutions;
    std::vector<int> seeds;
    size_t best_solution_idx;
    size_t worst_solution_idx;

    void add_solution(Solution solution);
    bool has_solution(const Solution &solution) const;
    void calculate_seeds(const Parameters &params);
    void join(const Population &a, const Population &b);
};

class SaDIWO {
  public:
    SaDIWO(Instance instance, Parameters params) : m_instance(std::move(instance)), m_params(std::move(params)) {}

    Solution solve();

  private:
    Instance m_instance;
    Parameters m_params;
    std::mt19937 &m_rng = RNG::instance().gen();

    std::vector<size_t> sort_inc_proc_time();

    // void update_departure_times(std::vector<long> &departures, size_t curr_job);
    [[nodiscard]] long get_idle_block_sum(const std::vector<long> &departures, size_t curr_job);

    Population spatial_dispersal(const Population &pop);
    size_t get_solution_d(const Population &pop, size_t solution_cost);
    Population competitive_exclusion(Population pop, Population new_pop) const;

    // Same as DIWO paper
    void local_search(Population &pop);
    Population population_init();
};

#endif
