#include "Core.h"

#include "Instance.h"
#include <algorithm>
#include <numeric>

void core::calculate_departure_times(Instance &instance, Solution &s) {
    auto p = [&instance](size_t i, size_t j) { return instance.p(i, j); };

    // Calculate first job
    s.departure_times[0][0] = p(s.sequence[0], 0);
    for (size_t j = 1; j < instance.num_machines(); j++) {
        s.departure_times[0][j] = s.departure_times[0][j - 1] + p(s.sequence[0], j);
    }

    for (size_t i = 1; i < s.sequence.size(); i++) {
        const size_t node = s.sequence[i];
        s.departure_times[i][0] = std::max(s.departure_times[i - 1][0] + p(node, 0), s.departure_times[i - 1][1]);
        for (size_t j = 1; j < instance.num_machines() - 1; j++) {

            const size_t current_finish_time = s.departure_times[i][j - 1] + p(node, j);

            s.departure_times[i][j] = std::max(current_finish_time, s.departure_times[i - 1][j + 1]);
        }
        s.departure_times[i].back() =
            s.departure_times[i][instance.num_machines() - 2] + p(node, instance.num_machines() - 1);
    }
}

void core::calculate_tail(Instance &instance, Solution &s) {

    auto p = [&instance](size_t i, size_t j) { return instance.p(i, j); };

    size_t last_index = s.sequence.size() - 1;
    // Calculate first job
    s.tail[last_index].back() = p(s.sequence.back(), instance.num_machines() - 1);
    for (long j = (long)instance.num_machines() - 2; j >= 0; j--) {
        s.tail[last_index][j] = s.tail[last_index][j + 1] + p(s.sequence.back(), j);
    }

    for (long i = ((long)s.sequence.size()) - 2; i >= 0; i--) {
        const size_t node = s.sequence[i];
        s.tail[i].back() = std::max(s.tail[i + 1].back() + p(node, instance.num_machines() - 1),
                                  s.tail[i + 1][instance.num_machines() - 2]);
        for (long j = (long)instance.num_machines() - 2; j >= 1; j--) {

            const size_t current_finish_time = s.tail[i][j + 1] + p(node, j);

            s.tail[i][j] = std::max(current_finish_time, s.tail[i + 1][j - 1]);
        }
        s.tail[i][0] = s.tail[i][1] + p(node, 0);
    }
}

std::vector<size_t> core::stpt_sort(Instance &instance) {
    std::vector<size_t> seq(instance.num_jobs());
    std::iota(seq.begin(), seq.end(), 0);

    std::sort(seq.begin(), seq.end(), [&instance](size_t a, size_t b) {
        return instance.processing_times_sum()[a] < instance.processing_times_sum()[b];
    });

    return seq;
}

size_t core::calculate_sigma(Instance &instance, std::vector<std::vector<size_t>> &d,
                             std::vector<size_t> &new_departure_time, size_t job, size_t k) {

    const size_t m = instance.num_machines(); // number of machines

    auto p = [&instance](size_t i, size_t j) { return instance.p(i, j); };

    size_t sigma = 0;

    for (size_t machine = 0; machine < m; machine++) {

        if (k == 0) {
            sigma += (new_departure_time[machine] - p(job, machine));
        } else {
            sigma += (new_departure_time[machine] - d[d.size() - 1][machine] - p(job, machine));
        }
    }

    return sigma;
}

std::vector<size_t> core::calculate_new_departure_time(Instance &instance, std::vector<std::vector<size_t>> &d,
                                                       size_t node) {

    const size_t m = instance.num_machines(); // number of machines

    auto p = [&instance](size_t i, size_t j) { return instance.p(i, j); };

    std::vector<size_t> new_departure_time(m);

    const size_t k_job = d.size() - 1;

    /* Calculating equal how to calculate any departure time */
    new_departure_time[0] = std::max((d[k_job][0]) + p(node, 0), d[k_job][1]);

    for (size_t j = 1; j < m - 1; j++) {

        const size_t current_finish_time = new_departure_time[j - 1] + p(node, j);

        new_departure_time[j] = std::max(current_finish_time, d[k_job][j + 1]);
    }

    new_departure_time.back() = new_departure_time[m - 2] + p(node, m - 1);

    return new_departure_time;
}

void core::partial_recalculate_solution(Instance &instance, Solution &s, size_t start) {
    if (start == 0) {
        recalculate_solution(instance, s);
        return;
    }

    auto p = [&instance](size_t i, size_t j) { return instance.p(i, j); };

    // Recalculate departure times from start index to the end
    for (size_t i = start; i < s.sequence.size(); i++) {
        const size_t node = s.sequence[i];
        s.departure_times[i][0] = std::max(s.departure_times[i - 1][0] + p(node, 0), s.departure_times[i - 1][1]);
        for (size_t j = 1; j < instance.num_machines() - 1; j++) {

            const size_t current_finish_time = s.departure_times[i][j - 1] + p(node, j);

            s.departure_times[i][j] = std::max(current_finish_time, s.departure_times[i - 1][j + 1]);
        }
        s.departure_times[i].back() =
            s.departure_times[i][instance.num_machines() - 2] + p(node, instance.num_machines() - 1);
    }

    s.cost = s.departure_times.back().back();
}

void core::recalculate_solution(Instance &instance, Solution &s) {

    if(s.sequence.size() > s.departure_times.size()){
        s.departure_times.resize(s.sequence.size());
        for(size_t i = 0; i < s.sequence.size(); i++){
            if(s.departure_times[i].empty()){
                s.departure_times[i].resize(instance.num_machines());
            }
        }
    }

    calculate_departure_times(instance, s);
    size_t last_index = s.sequence.size()-1 ;
    s.cost = s.departure_times[last_index].back();
}