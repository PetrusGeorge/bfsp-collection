#include "Core.h"

#include "Instance.h"
#include <algorithm>

std::vector<std::vector<size_t>>
core::calculate_departure_times(const Instance &instance, const std::vector<size_t> &sequence, bool jobs_reversed) {
    auto departure_times = std::vector(sequence.size(), std::vector<size_t>(instance.num_machines()));

    const std::function<long(size_t, size_t)> p = get_reversible_matrix(instance, jobs_reversed);

    // Calculate first job
    departure_times[0][0] = p(sequence[0], 0);
    for (size_t j = 1; j < instance.num_machines(); j++) {
        departure_times[0][j] = departure_times[0][j - 1] + p(sequence[0], j);
    }

    for (size_t i = 1; i < sequence.size(); i++) {
        const size_t node = sequence[i];
        departure_times[i][0] = std::max(departure_times[i - 1][0] + p(node, 0), departure_times[i - 1][1]);
        for (size_t j = 1; j < instance.num_machines() - 1; j++) {

            const size_t current_finish_time = departure_times[i][j - 1] + p(node, j);

            departure_times[i][j] = std::max(current_finish_time, departure_times[i - 1][j + 1]);
        }
        departure_times[i].back() =
            departure_times[i][instance.num_machines() - 2] + p(node, instance.num_machines() - 1);
    }

    return departure_times;
}

std::function<long(size_t, size_t)> core::get_reversible_matrix(const Instance &instance, bool jobs_reversed) {

    // A c++ hack using lambda functions to call normal matrix view or the m_reversed one
    std::function<long(size_t, size_t)> p = [&instance](size_t i, size_t j) { return instance.p(i, j); };
    if (jobs_reversed) {
        p = [&instance](size_t i, size_t j) { return instance.rp(i, j); };
    }

    return p;
}

// void core::stpt_sort(const Instance& instance, std::vector<size_t> &sequence, bool jobs_reversed) {
//     auto p = get_reversible_matrix(instance, jobs_reversed);

//     std::ranges::sort(sequence,[p, instance](size_t a, size_t b) {
//         size_t sum_a = 0;

//         for (size_t j = 0; j < instance.num_machines(); j++) {
//             sum_a += p(a, j);
//         }
//         size_t sum_b = 0;

//         for (size_t j = 0; j < instance.num_machines(); j++) {
//             sum_b += p(b, j);
//         }
//         return sum_a < sum_b;
//     });
// }
