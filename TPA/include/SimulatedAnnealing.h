#ifndef SIMULATED_ANNEALING_H
#define SIMULATED_ANNEALING_H

#include "Solution.h"

#include <iostream>

class SimulatedAnnealing {
  public:
    SimulatedAnnealing(Solution &solution, Instance &instance, double finalTemp, int nIter);
    Solution solve();

  private:
    Solution &solution;
    Instance instance;

    int nIter;

    double finalTemp;
    double initialTemp;
    double decay;

    void calculateInitialTemp();
    void calculateDecay();

};

#endif