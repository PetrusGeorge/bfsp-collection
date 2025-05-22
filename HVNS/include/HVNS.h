#ifndef HVNS_H
#define HVNS_H

#include "Core.h"
#include "Instance.h"
#include "Log.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"
#include "constructions/NEH.h"
#include "local-search/RLS.h"

#define KMAX 4 // number of neighborhoods

class HVNS {

  public:
    HVNS(Instance instance, Parameters params);

    // use neh to generate one solution
    Solution generate_first_solution();

    // Generate a random permutation of jobs
    std::vector<size_t> generate_random_sequence();

    // verify if two solutions are equal
    bool equal_solution(Solution &s1, Solution &s2);
    
    std::pair<size_t, size_t> taillard_best_edge_insertion(const std::vector<size_t> &sequence, std::pair<size_t, size_t> &jobs, size_t original_position);

    bool best_insertion(Solution &s);

    bool best_edge_insertion(Solution &s);

    bool best_swap(Solution &s);

    void shaking(Solution &s, size_t k);

    void sa_rls(Solution &s, Solution &best);

    void sa_best_edge_insertion(Solution &s, Solution &best);

    Solution solve();


  private:
    Instance m_instance;
    Parameters m_params;
    size_t m_time_limit;
    double m_T;
    double m_T_init;
    double m_T_fin;
    size_t m_n_iter;
};

#endif