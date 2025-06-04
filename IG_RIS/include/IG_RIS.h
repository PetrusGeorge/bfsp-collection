#ifndef IG_H
#define IG_H

#include "constructions/PF_NEH.h"
#include "local-search/RLS.h"
#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"

class IG {
  public:
    IG(Instance instance, Parameters params);
    Solution solve();

  private:
    // Random destroy solution using the parameter d defined on the params struct
    std::vector<size_t> destroy(Solution &s);

    // Constant Temperature to calculate acceptance criterion
    double acceptance_criterion_temperature()
    { return m_T = m_params.tP() * m_instance.all_processing_times_sum() / 10*m_instance.num_jobs()*m_instance.num_machines(); }

    // Calculate acceptance criterion to worse solution
    double acceptance_criterion(Solution&, Solution&);

    // These classes are "imutable" all their members are private and there is no "set" method for those members
    // almost like a read only memory
    Instance m_instance;
    Instance m_instance_reverse;
    Parameters m_params;

    // Constant temperature to acceptance criterion
    double m_T;
};

#endif
