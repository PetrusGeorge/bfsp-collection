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
    static Solution solve(MFFO &mffo_instance);
    static std::vector<size_t> min_max(const Instance &instance, const Parameters &param, bool jobs_reversed);
    static std::function<long(size_t, size_t)> get_reversible_matrix(const Instance &instance, bool reversed);

    const Instance &instance() const { return m_instance; }
    const Parameters &param() const { return m_param; }

  private:
    Instance m_instance;
    Parameters m_param;
};

#endif // MFFO_H
