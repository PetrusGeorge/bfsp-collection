#ifndef PFT_NEH_H
#define PFT_NEH_H

#include "Instance.h"
#include "Solution.h"

class PFT_NEH { // NOLINT
  public:
    PFT_NEH(Instance &instance);

    Solution solve(size_t lambda);

  private:
    Instance &m_instance;
};

#endif
