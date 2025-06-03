#ifndef DE_PLS_H
#define DE_PLS_H

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"
#include "constructions/NEH.h"

#include <vector>

class DE_PLS {

  public:
    DE_PLS(Instance instance, Parameters params);

    // Generate a random sequence using PF-NEH and other PS-1 randomly
    void initialize_population();

    // Generate a random permutation of jobs
    std::vector<size_t> generate_random_sequence();

    // Inserts a randomly chosen job into another position
    std::vector<double> get_mutant();

    void get_trial(std::vector<double> &x);

    // verify if two solutions are equal
    static bool equal_solution(Solution &s1, Solution &s2);

    static void update_params(Solution &s, std::vector<double> &x);

    static void perturbation(Solution &s);

    void desconstruct_construct(Solution &s);

    Solution solve();

  private:
    Instance m_instance;
    Parameters m_params;
    size_t m_time_limit;
    std::vector<Solution> m_pop;
    double m_T{};
    NEH m_helper;
};

#endif