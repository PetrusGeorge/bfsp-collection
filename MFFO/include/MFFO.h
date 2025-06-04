#ifndef MFFO_H
#define MFFO_H

#include <utility>

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"

class MFFO {
  public:
    MFFO(Instance instance, Parameters param) : m_instance(std::move(instance)), m_param(std::move(param)) {}
    Solution solve();

    std::vector<Solution> initialization();
    static Solution neighbourhood_search(Solution s);
    static void neighbourhood_insertion_first(Solution &s);
    static void neighbourhood_insertion_back(Solution &s);
    static void neighbourhood_swap(Solution &s);

  private:
    Instance m_instance;
    Parameters m_param;
};

#endif // MFFO_H
