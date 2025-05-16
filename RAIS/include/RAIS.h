#ifndef RAIS_H
#define RAIS_H

#include "Core.h"
#include "Instance.h"
#include "Log.h"
#include "RNG.h"
#include "Parameters.h"
#include "Solution.h"

class RAIS {

public:
  RAIS(Instance instance, Parameters params);

  double affinity_calculation(size_t cost);

  void pop_affinity_calculation(std::vector<Solution> &pop);

  std::vector<Solution> initialization();

  std::vector<Solution> clone_antibodies(const std::vector<Solution> &pop);

  void mutation(std::vector<Solution> &pop);

  bool nearby_antibody(Solution &s1, Solution &s2);

  void supression(std::vector<Solution> &pop);

  void SA(std::vector<Solution> &pop, double T);

  void select_nc_best(std::vector<Solution> &pop);

  Solution solve();

private:
  Instance m_instance;
  Parameters m_params;
  size_t time_limit;
};

#endif