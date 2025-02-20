#ifndef GRASP_H
#define GRASP_H

#include "Instance.h"
#include "Solution.h"

class GRASP {

  public:
    GRASP(Instance &instance);

    // construct a solution using the greed GRASP criterion
    Solution solve(double beta);

  private:
    Instance &m_instance;
};

#endif
