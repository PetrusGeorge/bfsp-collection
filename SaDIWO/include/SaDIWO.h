#ifndef SADIWO_H
#define SADIWO_H

#include "Instance.h"
#include "Solution.h"
#include <random>
#include "RNG.h"

// See end of section 5.4 on the SaDIWO paper
struct SaDIWOParams {
    size_t p_max{3};
    int s_min{0};
    int s_max{2};
    int sigma_min{3};
    int sigma_max{6};
};

// TODO: swap stuff for this guy
class Population {
  public:
    std::vector<Solution> solutions;
    std::vector<int> seeds;
    size_t best_solution_idx;
    size_t worst_solution_idx;

    void add_solution(const Solution &solution);
    void add_solutions(const std::vector<Solution> &solutions);
    bool has_solution(const Solution &solution) const;
    void calculate_seeds(const SaDIWOParams &params);
};

class SaDIWO {
  public:
    SaDIWO(Instance instance, SaDIWOParams params)
        : m_instance(std::move(instance)), m_params(params) {}

    Solution solve();

  private:
    Instance m_instance;
    SaDIWOParams m_params;
    std::mt19937 &m_rng = RNG::instance().gen();

    std::vector<size_t> sort_inc_proc_time();

    // void update_departure_times(std::vector<long> &departures, size_t curr_job);
    [[nodiscard]] long get_idle_block_sum(const std::vector<long> &departures, size_t curr_job);

    void spatial_dispersal(const Population &pop, Population &new_pop);
    size_t get_solution_d(const Population &pop, size_t solution_cost);

    void ls1(Solution &sol);
    void ls2(Solution &sol);
    void ls3(Solution &sol);
    void local_search(Population &pop);

    // Same as DIWO paper
    Population population_init();
    // void taillard_best_insert(Solution &final_sol, size_t q, size_t curr_job) const;
    // Solution neh(const std::vector<size_t> &seq) const;
    // std::vector<size_t> pf(const std::vector<size_t> &init_seq, size_t pos) const;
    // std::vector<std::vector<long>> compute_tail(const std::vector<size_t> &pi, size_t q) const;
    // std::vector<std::vector<long>> compute_completion_times(const std::vector<size_t> &pi, size_t q) const;
};

#endif
