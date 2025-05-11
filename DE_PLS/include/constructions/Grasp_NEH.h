#ifndef GRASP_NEH_H
#define GRASP_NEH_H

#include "Instance.h"
#include "Solution.h"

class GraspNeh {
  public:
    static Solution solve(int x, double beta, Instance &instance);
};

#endif
