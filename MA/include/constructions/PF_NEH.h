#ifndef PF_NEH_H
#define PF_NEH_H

#include "Instance.h"
#include "Solution.h"

class PF_NEH { // NOLINT
  public:
    PF_NEH(Instance &instance);

    Solution solve(size_t lambda);

  private:
    Instance &m_instance;
};

#endif