#ifndef CORE_H
#define CORE_H

#include "Instance.h"
#include <functional>

namespace core {
    std::vector<std::vector<size_t>> calculate_departure_times(const Instance& instance, const std::vector<size_t> &sequence, bool jobs_reversed = false);

    // Easy hack to implement algorithms using both direct and reverse instances
    std::function<long(size_t, size_t)> get_reversible_matrix(const Instance& instance, bool jobs_reversed);
    
}

#endif // !CORE_H
