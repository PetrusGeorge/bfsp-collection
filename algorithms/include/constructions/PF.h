#ifndef PF_H
#define PF_H

#include "Instance.h"
#include "Solution.h"

class PF {
  public:
    PF(Instance &instance);
    Solution solve();
    void pf_insertion_phase(Solution& s, size_t first_job);

  private:
    Instance& m_instance;
};

#endif
