#ifndef SIMULATED_ANNEALING_H
#define SIMULATED_ANNEALING_H

#include "Instance.h"
#include "Solution.h"

#include <iostream>

class SimulatedAnnealing {
  public:
    SimulatedAnnealing(Solution &solution, Instance &instance, double final_temp, int n_iter);
    Solution solve();

  private:
    Solution &m_solution;
    Instance &m_instance;

    int m_n_iter;

    double m_final_temp;
    double m_initial_temp;
    double m_decay;

    void calculate_initial_temp();
    void calculate_decay();
    Solution anneal(Solution &current_solution, size_t position);
};

#endif
