#ifndef MA_H
#define MA_H

#include "Core.h"
#include "Instance.h"
#include "Log.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"
#include "constructions/PF_NEH.h"
#include "local-search/RLS.h"

#include <vector>

#define LAMBDA_MAX 20 // parameters given in https://doi.org/10.1109/TASE.2012.2219860

class MA {

  public:
    MA(Instance instance, Parameters params);

    // Generate a random sequence using PF-NEH and other PS-1 randomly
    void generate_initial_pop();

    // Generate a random permutation of jobs
    std::vector<size_t> generate_random_sequence();

    // Select to individuals randomly and return the index of the one with better makespan
    size_t selection();

    /*
    Apply swap moviments on beta to turn it equal to pi.
    All intermediate solutions are evaluated and the best one is returned.
    pi and beta aren't considered, and if the 2 solution are equal, beta is modified
    */
    Solution path_relink_swap(const Solution &beta, const Solution &pi);

    // Inserts a randomly chosen job into another position
    void mutation(Solution &individual);

    // Mutate some solution and creates new ones randomly
    void restart_population();

    // verify if two solutions are equal
    bool equal_solution(Solution &s1, Solution &s2);

    /*
    substituting some individuals by offsprings.
    This algorithm only accept new solution, with better makespan
    */
    void population_updating(std::vector<Solution> &offspring_population);

    Solution solve();

  private:
    Instance m_instance;
    Parameters m_params;
    size_t m_time_limit;
    std::vector<Solution> m_pop;
};

#endif
