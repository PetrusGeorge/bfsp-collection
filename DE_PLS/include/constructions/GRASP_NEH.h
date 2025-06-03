#ifndef GRASP_NEH_H
#define GRASP_NEH_H

#include "Instance.h"
#include "Solution.h"

class GRASP_NEH {
  public:
    GRASP_NEH(Instance &instance, size_t x, double beta);

    Solution solve();

    Solution GRASP();

  private:
    Instance &m_instance;
    size_t m_x;
    double m_beta;
};

#endif
