#ifndef PF_NEH_H
#define PF_NEH_H

#include "Instance.h"
#include "Solution.h"

class PFNeh {
  public:
    static Solution solve(size_t x, size_t delta, Instance &instance);
};

#endif
