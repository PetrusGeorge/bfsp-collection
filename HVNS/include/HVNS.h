#ifndef HVNS_H
#define HVNS_H

#include "Core.h"
#include "Instance.h"
#include "Log.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"
#include "constructions/NEH.h"

#define KMAX 3 // number of neighborhoods

class HVNS {

  public:
    HVNS(Instance instance, Parameters params);

    // use neh to generate one solution
    Solution generate_first_solution();

    // Generate a random permutation of jobs
    std::vector<size_t> generate_random_sequence();

    // verify if two solutions are equal
    static bool equal_solution(Solution &s1, Solution &s2);

    // adaptation of taillard_best_insertion to edge insertion method
    std::pair<size_t, size_t> taillard_best_edge_insertion(const std::vector<size_t> &sequence,
                                                           std::pair<size_t, size_t> &jobs, size_t original_position);

    // finds the best position to reinsert a job and reinserts it
    void best_insertion(Solution &s);

    // finds the best position to reinsert a sequence of two jobs and reinserts them
    void best_edge_insertion(Solution &s);

    // finds the best job swap and applies it
    void best_swap(Solution &s);

    // it defines which method (swap, edge insertion, insertion) will be used
    void shaking(Solution &s, size_t k);

    // rls + sa = it can accept some insertions, even if it's worth than the current solution
    void sa_rls(Solution &s, Solution &best);

    // edge insertion + sa = it can accept some edge insertions movements, even if it's worth than the current solution
    void sa_best_edge_insertion(Solution &s, Solution &best);

    Solution solve();

  private:
    Instance m_instance;
    Parameters m_params;
    size_t m_time_limit;
    double m_T;      // current temperature
    double m_T_init; // initial temperature
    double m_T_fin;  // "final" temperature
    double m_beta;   // cooling adjustment
};

#endif