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

  void pop_affinity_calculation(std::vector<std::pair<Solution, double>> &pop);

  std::vector<std::pair<Solution, double>> initial_pop();

  std::vector<std::pair<Solution, double>>
  clone_antibodies(std::vector<std::pair<Solution, double>> &pop);

  void mutation(std::vector<std::pair<Solution, double>> &pop);

  bool nearby_antibody(Solution &s1, Solution &s2);

  void supression(std::vector<std::pair<Solution, double>> &pop,
                  std::vector<std::pair<Solution, double>> &clones);

  void update(std::vector<std::pair<Solution, double>> &pop,
              std::vector<std::pair<Solution, double>> &clones);

  void SA(std::vector<std::pair<Solution, double>> &pop, double T);

  Solution solve();

private:
  Instance m_instance;
  Parameters m_params;
  double time_limit;
};

#endif