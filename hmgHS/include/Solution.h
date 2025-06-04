#ifndef SOLUTION_H
#define SOLUTION_H

#include <limits>
#include <ostream>
#include <vector>

struct Solution {
    size_t cost = std::numeric_limits<size_t>::max();
    std::vector<size_t> sequence;
    std::vector<double> harmony;
    std::vector<std::vector<size_t>> departure_times;
    std::vector<std::vector<size_t>> tail;
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

#endif