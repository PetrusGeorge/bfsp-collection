#ifndef CORE_H
#define CORE_H

#include "Instance.h"
#include "Solution.h"

namespace core {

std::vector<std::vector<size_t>>
calculate_departure_times(const Instance &instance,
                          const std::vector<size_t> &sequence);

std::vector<size_t> stpt_sort(const Instance &instance);

std::vector<size_t>
calculate_new_departure_time(const Instance &instance,
                             std::vector<std::vector<size_t>> &d, size_t node);

size_t calculate_sigma(const Instance &instance,
                       std::vector<std::vector<size_t>> &d,
                       std::vector<size_t> &new_departure_time, size_t job,
                       size_t k);

void recalculate_solution(const Instance &instance, Solution &s);

} // namespace core

#endif // !CORE_H
