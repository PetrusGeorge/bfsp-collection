#ifndef IG_H
#define IG_H

#include "constructions/PF_NEH.h"
#include "local-search/RLS.h"
#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"

class IG_IJ {
  public:
    IG_IJ(Instance instance, Parameters params);
    Solution solve();

  private:
    // Random destroy solution using the parameter d defined on the params struct
    std::vector<size_t> destroy(Solution &s);

    double acceptance_criterion_temperature()
    { return m_T = m_params.tP() * m_instance.all_processing_times_sum() / 10*m_instance.num_jobs()*m_instance.num_machines(); }
    
    double acceptance_criterion(Solution, Solution);

    void BestSwap(Solution &solution);

    // These classes are "imutable" all their members are private and there is no "set" method for those members
    // almost like a read only memory
    Instance m_instance;
    Instance m_instance_reverse;
    Parameters m_params;

    // constant temperature to acceptance
    double m_T;
};

#endif
