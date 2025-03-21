#ifndef MFFO_H
#define MFFO_H

#include <functional>
#include <utility>
#include <vector>

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"

class MFFO {
  public:
    MFFO(Instance instance, Parameters param) : m_instance(std::move(instance)), m_param(std::move(param)) {}
    Solution solve();

    static std::vector<size_t> min_max(const Instance &instance, const Parameters &param, bool jobs_reversed);
    static std::function<long(size_t, size_t)> get_reversible_matrix(const Instance &instance, bool reversed);

    static Solution neighbourhood_search(Solution &s);
    static void neighbourhood_insertion_first(Solution s);
    static void neighbourhood_insertion_back(Solution s);
    static void neighbourhood_swap(Solution s);

    Instance &instance() { return m_instance; }
    Parameters &param() { return m_param; }

  private:
    Instance m_instance;
    Parameters m_param;
};

#endif // MFFO_H
