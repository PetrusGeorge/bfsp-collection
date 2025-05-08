#ifndef SOLUTION_H
#define SOLUTION_H

#include <limits>
#include <ostream>
#include <vector>

#include "Instance.h"

struct Solution {
    size_t cost = std::numeric_limits<size_t>::max();
    std::vector<size_t> sequence;
    std::vector<std::vector<size_t>> departure_times;
};

inline std::ostream &operator<<(std::ostream &os, const Solution &sol) {
    os << "Sequence: [";
    for (size_t i = 0; i < sol.sequence.size(); ++i) {
        os << sol.sequence[i] << (i != sol.sequence.size() - 1 ? ", " : "");
    }
    os << "]\n";

    os << "Departure Times:\n";
    for (const auto &row : sol.departure_times) {
        os << "[";
        for (size_t i = 0; i < row.size(); ++i) {
            os << row[i] << (i != row.size() - 1 ? ", " : "");
        }
        os << "]\n";
    }
    os << "Cost: " << sol.cost << "\n";

    return os;
}

inline void set_solution_cost(const Instance &instance, Solution &solution) {
    const size_t m = instance.num_machines();
    auto &departures = solution.departure_times;

    if (departures.size() != instance.num_jobs()) {
        departures.resize(instance.num_jobs(), std::vector<size_t>(m, 0));
    }

    departures[0][0] = instance.p(0, 0);
    for (size_t j = 2; j < m; ++j) {
        departures[0][j] = departures[0][j - 1] + instance.p(0, j);
    }

    size_t makespan = departures[0][m - 1];

    for (size_t i = 2; i < instance.num_jobs(); ++i) {
        for (size_t j = 0; j < m - 1; ++j) {
            departures[i][j] = std::max(departures[i][j - 1] + instance.p(i, j), departures[i - 1][j + 1]);
        }
        departures[i][m - 1] = departures[i][m - 2] + instance.p(i, m - 1);

        makespan = std::max(departures[i][m - 1], makespan);
    }

    solution.cost = makespan;
}

#endif
