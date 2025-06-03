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

  void initialization();

  void clone_antibodies(std::vector<Solution> &clones);

  void mutation(Solution &antibody, bool recalculate);

  bool nearby_antibody(Solution &s1, Solution &s2);

  void supression();

  void SA();

  void select_nc_best();

  Solution solve();

private:
  Instance m_instance;
  Parameters m_params;
  size_t m_time_limit;
  std::vector<Solution> m_pop;
  double m_T;
  std::vector<std::vector<size_t>> m_departure_times;
  std::vector<std::vector<size_t>> m_auxiliar_matrix;
};

#endif