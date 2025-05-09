#ifndef CORE_H
#define CORE_H

#include "Instance.h"
#include "Solution.h"

namespace core {

std::vector<std::vector<size_t>> calculate_departure_times(Instance &instance, const std::vector<size_t> &sequence);

std::vector<std::vector<size_t>> calculate_tail(Instance &instance, const std::vector<size_t> &sequence);

std::vector<size_t> stpt_sort(Instance &instance);

std::vector<size_t> calculate_new_departure_time(Instance &instance, std::vector<std::vector<size_t>> &d, size_t node);

size_t calculate_sigma(Instance &instance, std::vector<std::vector<size_t>> &d, std::vector<size_t> &new_departure_time,
                       size_t job, size_t k);

void partial_recalculate_solution(Instance &instance, Solution &s, size_t start);

void recalculate_solution(Instance &instance, Solution &s);

} // namespace core

#endif // !CORE_H
