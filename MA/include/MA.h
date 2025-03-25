#ifndef MA_H
#define MA_H

#include "Core.h"
#include "Instance.h"
#include "Log.h"
#include "Solution.h"
#include "Parameters.h"
#include "RNG.h"
#include "local-search/RLS.h"
#include "constructions/PF_NEH.h"

#include <vector>

#define LAMBDA_MAX 20

class MA {
  public:
    MA(Instance instance, Parameters params);
    void generate_initial_pop();
    std::vector<size_t> generate_random_sequence();
    size_t selection();
    Solution path_relink_swap(const Solution &individual1, const Solution &individual2);
    void mutation(Solution &individual);
    void restart_population();
    bool equal_solution(Solution &s1, Solution &s2);
    void population_updating(std::vector<Solution> &offspring_population);
    Solution solve();

  private:
    Instance m_instance;
    Parameters m_params;
    size_t time_limit;
    std::vector<Solution> pop;
};

#endif 
