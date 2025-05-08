#pragma once

#ifndef SADIWO_H
#define SADIWO_H

#include <random>

#include "Instance.h"
#include "Solution.h"

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
    SaDIWO(const Instance &instance, const SaDIWOParams &params)
        : m_instance(instance), m_params(params), m_rng(m_rd()) {}

    Solution solve();

  private:
    const Instance &m_instance;
    const SaDIWOParams &m_params;
    std::random_device m_rd;
    std::mt19937 m_rng;

    std::vector<size_t> sort_inc_proc_time() const;

    void update_departure_times(std::vector<long> &departures, size_t curr_job) const;
    [[nodiscard]] long get_idle_block_sum(const std::vector<long> &departures, size_t curr_job) const;

    void spatial_dispersal(const Population &pop, Population &new_pop);
    size_t get_solution_d(const Population &pop, size_t solution_cost);

    void ls1(Solution &sol) const;
    void ls2(Solution &sol) const;
    void ls3(Solution &sol) const;
    void local_search(Population &pop) const;

    // Same as DIWO paper
    Population population_init() const;
    void taillard_best_insert(Solution &final_sol, size_t q, size_t curr_job) const;
    Solution neh(const std::vector<size_t> &seq) const;
    std::vector<size_t> pf(const std::vector<size_t> &init_seq, size_t pos) const;
    std::vector<std::vector<long>> compute_tail(const std::vector<size_t> &pi, size_t q) const;
    std::vector<std::vector<long>> compute_completion_times(const std::vector<size_t> &pi, size_t q) const;
};

#endif
