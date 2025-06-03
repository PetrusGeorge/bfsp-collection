#ifndef MINMAX_H
#define MINMAX_H
#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"

class MinMax{
  public:
    // alpha parameter value got from Roconi paper, https://doi.org/10.1016/S0925-5273(03)00065-3
    MinMax(Instance& instance, double alpha = 0.60);
    Solution solve();

  private:
    Instance m_instance;
    double m_alpha;

};

#endif
