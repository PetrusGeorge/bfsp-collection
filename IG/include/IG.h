#ifndef IG_H
#define IG_H

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"
#include <functional>

class IG {
  public:
    IG(Instance &&instance, Parameters &&params);
    Solution solve();

  private:
    Solution initial_solution();
    std::vector<size_t> min_max(bool reverse = false);
    Solution neh(std::vector<size_t>&& phi, bool reverse = false);
    Solution mme2();
    std::function<long(size_t, size_t)> get_reversible_matrix(bool reverse);
    // Solution mme_reverse();
    static Solution local_search(Solution s);
    void destroy(Solution &s);
    void construct(Solution &s);

    void recalculate_solution(Solution &s);
    std::vector<std::vector<size_t>> calculate_departure_times(const std::vector<size_t>& sequence, bool reverse = false);

    Instance m_instance;
    Parameters m_params;

    Solution m_best;
};

#endif
