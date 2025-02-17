#ifndef GRASP_NEH_H
#define GRASP_NEH_H

#include "Instance.h"
#include "Solution.h"

class GraspNeh {
  public:
    static Solution solve(int x, int delta, const Instance &instance);
    
};

#endif
