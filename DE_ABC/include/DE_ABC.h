#ifndef DEABC_H
#define DEABC_H

#include "Core.h"
#include "Instance.h"
#include "Log.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"
#include "constructions/NEH.h"
#include "constructions/MinMax.h"
#include "local-search/RLS.h"

#include <vector>

class DE_ABC {

  public:
    DE_ABC(Instance instance, Parameters params);

    bool new_in_population(std::vector<size_t> &sequence);

    void generate_initial_pop();

    std::vector<size_t> generate_random_sequence();

    size_t selection();

    std::vector<size_t> mutation();

    Solution crossover(std::vector<size_t> &pi);

    void update_neighborhood();

    void swap(Solution &s);

    void insertion(Solution &s);

    void self_adaptative();

    void updating_unchanged();

    void replace_worst_solution(Solution &s);

    Solution solve();

  private:
    Instance m_instance;
    Parameters m_params;
    size_t m_time_limit;
    std::vector<Solution> m_pop;
    std::vector<bool> changed;
    std::vector<size_t> BNL;
    std::vector<size_t> NL;
};

#endif