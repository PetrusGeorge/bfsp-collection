#ifndef SVNS_D_H
#define SVNS_D_H

#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"
#include <cstddef>
#include <vector>

class SVNS_D { // NOLINT
  private:
    Instance &m_instance;
    Parameters &m_parameters;

    void apply_insertion(Solution &solution, long from, long to);

  public:
    Solution PW_PWE2(); // NOLINT

    void LS1_D_swap(Solution &solution, std::vector<size_t> &reference); // NOLINT
    void LS1_D(Solution &solution);                                      // NOLINT

    void LS2_D_insertion(Solution &solution, std::vector<size_t> &reference); // NOLINT
    void LS2_D(Solution &solution);                                           // NOLINT

    SVNS_D(Instance &instance, Parameters &params);

    Solution solve();
};

#endif
