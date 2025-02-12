#include "algorithms/LPT.h"
#include "Core.h"
#include "Instance.h"
#include <algorithm>
#include <numeric>

std::vector<size_t> initial_job_sequence(const Instance &instance, bool jobs_reversed = false) {
    std::vector<size_t> sequence(instance.num_jobs());
    std::iota(sequence.begin(), sequence.end(), 0);

    auto p = core::get_reversible_matrix(instance, jobs_reversed);

    std::vector<size_t> optzado;
    optzado.reserve(instance.num_jobs());

    for(size_t i = 0; i < instance.num_jobs(); i++){
        size_t sum = 0;
        for(size_t j = 0; j < instance.num_machines(); j++){
            sum += instance.p(i, j);
        }
        optzado.push_back(sum);
    }

    std::sort(sequence.begin(), sequence.end(), [optzado](size_t a, size_t b) {
        return optzado[a] > optzado[b];
    });

    return sequence;
}

Solution LPT::solve(const Instance &instance, bool jobs_reversed) {
    Solution s;

    s.sequence = initial_job_sequence(instance, jobs_reversed);
    return s;
}
