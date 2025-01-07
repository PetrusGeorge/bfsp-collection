#ifndef IG_H
#define IG_H

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"

class IG {
  public:
    IG(Instance &&instance, Parameters &&params);
    Solution solve();

  private:
    Solution initial_solution();
    Solution local_search(Solution s);
    void destroy(Solution &s);
    void construct(Solution &s);

    Instance m_instance;
    Parameters m_params;

    Solution m_best;
};

#endif
