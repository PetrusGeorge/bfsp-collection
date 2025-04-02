#ifndef SVNS_D_H
#define SVNS_D_H

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"

class SVNS_D { // NOLINT
  private:
    Instance &m_instance;
    Parameters &m_parameters;

    Solution PW_PWE2(); // NOLINT

  public:
    SVNS_D(Instance &instance, Parameters &params);

    Solution solve();
};

#endif
