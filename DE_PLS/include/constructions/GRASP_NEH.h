#ifndef GRASP_NEH_H
#define GRASP_NEH_H

#include "Instance.h"
#include "Solution.h"

class GraspNeh {
  public:
    GraspNeh(Instance &instance, size_t x, double beta);

    Solution solve();

    Solution grasp();

  private:
    Instance &m_instance;
    size_t m_x;
    double m_beta;
};

#endif
