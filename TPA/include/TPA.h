#ifndef TPA_H
#define TPA_H

#include "Instance.h"
#include "Solution.h"
#include "SimulatedAnnealing.h"

#include <iostream>

class TPA {
  public:
    TPA(Instance &instance);
    Solution solve();

  private:
    Instance &instance;

};

#endif