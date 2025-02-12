#ifndef LPT_H
#define LPT_H

#include "Solution.h"
#include "Instance.h"

class LPT {
  public:
    static Solution solve(const Instance &instance, bool jobs_reversed = false);
};

#endif
