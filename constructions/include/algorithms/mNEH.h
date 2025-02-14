#ifndef MNEH_H
#define MNEH_H

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"

class MNEH {
  private:
    static double average(size_t job, Instance &instance, bool jobs_reversed);
    static double standard_deviation(size_t job, Instance &instance, bool jobs_reversed);
    static std::vector<size_t> priority_rule(double alpha, Instance &instance, bool jobs_reversed);

  public:
    static Solution solve(double alpha, Instance &instance, Parameters &params, bool jobs_reversed);
};

#endif // !H
