#ifndef MNEH_H
#define MNEH_H

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"

class MNEH {
private:
  static double average(size_t job, Instance &instance);
  static double standard_deviation(size_t job, Instance &instance);
  static std::vector<size_t> priority_rule(double alpha, Instance &instance);

public:
  static Solution solve(double alpha, Instance &instance);
};

#endif // !H
