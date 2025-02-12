#ifndef PF_H
#define PF_H

#include "Instance.h"
#include "Solution.h"

class PF {
  public:
    static std::vector<size_t> stpt_sort(const Instance &instance);
    static Solution solve(const Instance &instance);
};

#endif
