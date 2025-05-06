#ifndef IG_H
#define IG_H

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"

class IG {
  public:
    IG(Instance instance, Parameters params);
    Solution solve();

  private:
    // This is a implementation of the mme2 algorithm, a variation of neh using min max as the first step
    Solution initial_solution();

    // Pretty straight foward local search
    Solution local_search(Solution s);
    // The name also implies here
    bool swap_first_improvement(Solution &s);

    // Random destroy solution using the parameter d defined on the params struct
    std::vector<size_t> destroy(Solution &s);

    // These classes are "imutable" all their members are private and there is no "set" method for those members
    // almost like a read only memory
    Instance m_instance;
    Instance m_instance_reverse;
    Parameters m_params;
};

#endif
