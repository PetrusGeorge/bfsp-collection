#include "algorithms/mNEH.h"

#include "Parameters.h"
#include "algorithms/NEH.h"

#include <cmath>
#include <cstddef>
#include <vector>

double MNEH::average(const size_t job, Instance &instance) {
    auto processing_time_matrix = [&instance](size_t i, size_t j) { return instance.p(i, j); };

    double sum = 0;

    for (size_t machine = 0; machine < instance.num_machines(); ++machine) {
        sum += static_cast<double>(processing_time_matrix(job, machine));
    }
    const double average = sum / static_cast<double>(instance.num_machines());

    return average;
}

double MNEH::standard_deviation(const size_t job, Instance &instance) {
    auto processing_time_matrix = [&instance](size_t i, size_t j) { return instance.p(i, j); };

    double sum = 0;

    for (size_t machine = 0; machine < instance.num_machines(); ++machine) {
        const double deviation = static_cast<double>(processing_time_matrix(job, machine)) - average(job, instance);
        sum += deviation * deviation;
    }
    const double standard_deviation = sqrt(sum / (static_cast<double>(instance.num_machines()) - 1));

    return standard_deviation;
}

std::vector<size_t> MNEH::priority_rule(const double alpha, Instance &instance) {
    std::vector<std::pair<size_t, double>> priority_rule_values;
    std::vector<size_t> initial_sequence;

    priority_rule_values.reserve(instance.num_jobs());
    initial_sequence.reserve(instance.num_machines());

    for (size_t job = 0; job < instance.num_jobs(); ++job) {
        const double priority_value =
            (alpha * average(job, instance)) + ((1 + alpha) * standard_deviation(job, instance));
        priority_rule_values.emplace_back(job, priority_value);
    }

    auto criteria = [](std::pair<size_t, double> a, std::pair<size_t, double> b) { return a.second < b.second; };

    std::sort(priority_rule_values.begin(), priority_rule_values.end(), criteria);

    for (const auto &machine : priority_rule_values) {
        initial_sequence.push_back(machine.first);
    }

    return initial_sequence;
}

Solution MNEH::solve(const double alpha, Instance &instance, Parameters &params) {
    const std::vector<size_t> priority_rule_sequence = priority_rule(alpha, instance);

    NEH neh(priority_rule_sequence, instance, params);

    Solution solution = neh.solve();

    return solution;
}
