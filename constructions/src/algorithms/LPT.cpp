#include "algorithms/LPT.h"
#include "Core.h"
#include "Instance.h"
#include <algorithm>
#include <numeric>

std::vector<size_t> initial_job_sequence(const Instance &instance) {
    std::vector<size_t> sequence(instance.num_jobs());
    std::iota(sequence.begin(), sequence.end(), 0);

    const std::vector<size_t> &pts = instance.processing_times_sum();

    std::sort(sequence.begin(), sequence.end(), [pts](size_t a, size_t b) { return pts[a] > pts[b]; });

    return sequence;
}

Solution LPT::solve(const Instance &instance) {
    Solution s;

    s.sequence = initial_job_sequence(instance);
    core::recalculate_solution(instance, s);
    return s;
}
