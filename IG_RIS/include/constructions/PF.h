#ifndef PF_H
#define PF_H

#include "Instance.h"
#include "Solution.h"

class PF {
  public:
    PF(Instance &instance);
    Solution solve();

    // insertion phase of pf in case anyone wants to do this with diferent start jobs
    // using solve() the first job will be the one with smallest total processing time
    void pf_insertion_phase(Solution &s, size_t first_job);

  private:
    Instance &m_instance;
};

#endif