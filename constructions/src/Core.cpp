#include "Core.h"

#include "Instance.h"
#include <algorithm>
#include <numeric>

std::vector<std::vector<size_t>> core::calculate_departure_times(const Instance &instance,
                                                                 const std::vector<size_t> &sequence) {
    auto departure_times = std::vector(sequence.size(), std::vector<size_t>(instance.num_machines()));

    auto p = [&instance](size_t i, size_t j) { return instance.p(i, j); };

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

std::vector<size_t> core::stpt_sort(const Instance &instance) {
    std::vector<size_t> seq(instance.num_jobs());
    std::iota(seq.begin(), seq.end(), 0);

    std::vector<size_t> optzado;
    optzado.reserve(instance.num_jobs());

    for (size_t i = 0; i < instance.num_jobs(); i++) {
        size_t sum = 0;
        for (size_t j = 0; j < instance.num_machines(); j++) {
            sum += instance.p(i, j);
        }
        optzado.push_back(sum);
    }

    std::sort(seq.begin(), seq.end(), [optzado, instance](size_t a, size_t b) { return optzado[a] < optzado[b]; });

    return seq;
}

size_t core::calculate_sigma(const Instance &instance, std::vector<std::vector<size_t>> &d,
                             std::vector<size_t> &new_departure_time, size_t job, size_t k) {

    const size_t m = instance.num_machines(); // number of machines

    auto p = [&instance](size_t i, size_t j) { return instance.p(i, j); };

    size_t sigma = 0;

    for (size_t machine = 0; machine < m; machine++) {
        size_t sum = 0;

        if (k == 0) {
            sum = (new_departure_time[machine] - p(job, machine));
        } else {
            sum = (new_departure_time[machine] - d[d.size() - 1][machine] - p(job, machine));
        }

        sigma += sum;
    }

    return sigma;
}

std::vector<size_t> core::calculate_new_departure_time(const Instance &instance, std::vector<std::vector<size_t>> &d,
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

void core::recalculate_solution(const Instance &instance, Solution &s) {
    s.departure_times = calculate_departure_times(instance, s.sequence);
    s.cost = s.departure_times.back().back();
}
