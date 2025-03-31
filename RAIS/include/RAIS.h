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

  std::vector<Solution> initial_pop();

  std::vector<Solution> clone_antibodies(const std::vector<Solution> &pop);

  void mutation(std::vector<Solution> &pop);

  bool nearby_antibody(Solution &s1, Solution &s2);

  void merge_populations(std::vector<Solution> &pop, const std::vector<Solution> &clones, const std::vector<bool> &pop_eliminated, const std::vector<bool> &clone_eliminated);

  void supression(std::vector<Solution> &pop,
                  std::vector<Solution> &clones);

  void update(std::vector<Solution> &pop,
              const std::vector<Solution> &clones);

  void SA(std::vector<Solution> &pop, double T);

  Solution solve();

private:
  Instance m_instance;
  Parameters m_params;
  size_t time_limit;
};

#endif