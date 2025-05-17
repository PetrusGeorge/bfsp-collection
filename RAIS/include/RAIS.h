#ifndef RAIS_H
#define RAIS_H

#include "Core.h"
#include "Instance.h"
#include "Log.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"

class RAIS {

public:
  RAIS(Instance instance, Parameters params);

  double affinity_calculation(size_t cost);

  void pop_affinity_calculation(std::vector<Solution> &set);

  std::vector<Solution> initialization();

  std::vector<Solution> clone_antibodies(std::vector<Solution> &clones);

  void mutation(std::vector<Solution> &set);

  bool nearby_antibody(Solution &s1, Solution &s2);

  void supression();

  void SA();

  void select_nc_best();

  Solution solve();

private:
  Instance m_instance;
  Parameters m_params;
  size_t time_limit;
  std::vector<Solution> pop;
  double T;
};

#endif