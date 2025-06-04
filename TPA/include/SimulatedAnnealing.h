#ifndef SIMULATED_ANNEALING_H
#define SIMULATED_ANNEALING_H

#include "Instance.h"
#include "Solution.h"
#include "Parameters.h"

#include <iostream>

class SimulatedAnnealing {
  public:
    SimulatedAnnealing(Solution &solution, Instance &instance, Parameters &params);
    Solution solve();

  private:
    Solution &m_solution;
    Instance &m_instance;
    Parameters &m_params;

    int m_n_iter;

    double m_final_temp;
    double m_initial_temp;
    double m_decay;

    void calculate_initial_temp();
    void calculate_decay();
    Solution anneal(Solution &current_solution, size_t position);
    std::vector<size_t> generate_random_sequence();
};

#endif
