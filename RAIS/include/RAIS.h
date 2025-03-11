#ifndef RAIS_H
#define RAIS_H

#include "Core.h"
#include "Instance.h"
#include "Log.h"
#include "RNG.h"
#include "Solution.h"

class RAIS {

  public: 

    RAIS(Instance instance, double alp, size_t threshold, size_t gt, size_t n);
  
    double affinity_calculation(size_t cost);
  
    void pop_affinity_calculation(std::vector<std::pair<Solution, double>> &pop);
  
    std::vector<std::pair<Solution, double>> initial_pop();
  
    std::vector<std::pair<Solution, double>> clone_antibodies(std::vector<std::pair<Solution, double>> &pop);
    
    void mutation(std::vector<std::pair<Solution, double>> &pop);
  
    bool nearby_antibody(Solution &s1, Solution &s2);
  
    void supression(std::vector<std::pair<Solution, double>> &pop, std::vector<std::pair<Solution, double>> &clones);
  
    void update(std::vector<std::pair<Solution, double>> &pop, std::vector<std::pair<Solution, double>> &clones);
  
    void SA(std::vector<std::pair<Solution, double>> &pop, double T);
  
    Solution solve();

  private:
    Instance m_instance;
    double alpha;
    size_t d_threshold;
    size_t Gt;
    size_t nc;
    double time_limit;
};

#endif