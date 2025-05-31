#ifndef TPA_H
#define TPA_H

#include "Instance.h"
#include "Solution.h"
#include "Parameters.h"
#include "SimulatedAnnealing.h"

#include <iostream>

class TPA {
  public:
    TPA(Instance &instance, Parameters &params);
    Solution solve();

  private:
    Instance &instance;
    Parameters &params;

};

#endif