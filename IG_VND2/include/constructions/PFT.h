#ifndef PFT_H
#define PFT_H

#include "Instance.h"
#include "Solution.h"

class PFT {
  public:
    PFT(Instance &instance);
    Solution solve();

    // insertion phase of pf in case anyone wants to do this with diferent start jobs
    // using solve() the first job will be the one with smallest total processing time
    void pft_insertion_phase(Solution &s, size_t first_job);

  private:
    Instance &m_instance;
};

#endif
