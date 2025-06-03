#ifndef CORE_H
#define CORE_H

#include "Instance.h"
#include "Solution.h"

namespace core {

void calculate_departure_times(Instance &instance, Solution &s);

void calculate_tail(Instance &instance, Solution &s);

std::vector<size_t> stpt_sort(Instance &instance);

std::vector<size_t> calculate_new_departure_time(Instance &instance, std::vector<std::vector<size_t>> &d, size_t node);

size_t calculate_sigma(Instance &instance, std::vector<std::vector<size_t>> &d, std::vector<size_t> &new_departure_time,
                       size_t job, size_t k);

void partial_recalculate_solution(Instance &instance, Solution &s, size_t start);

void recalculate_solution(Instance &instance, Solution &s);

} // namespace core

#endif // !CORE_H
