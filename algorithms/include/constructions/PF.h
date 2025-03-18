#ifndef PF_H
#define PF_H

#include "Instance.h"
#include "Solution.h"

class PF {
public:
  static Solution solve(const Instance &instance);
};

#endif
