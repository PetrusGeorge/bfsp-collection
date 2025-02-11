#ifndef SOLUTION_H
#define SOLUTION_H

#include <limits>
#include <ostream>
#include <vector>

struct Chromosome {
    double ds;  // destruction size
    double ps;  // perturbation strength
    double tau; // temperature adjustment parameter
    double jP;  // algorithm jumping probability

    std::vector<size_t> permutation;

    Chromosome(double ds = 5.0, double ps = 2.5, double tau = 0.5, double jp = 0.5)
        : ds(ds), ps(ps), tau(tau), jP(jp) {}
};

struct Solution {
    size_t cost = std::numeric_limits<size_t>::max();
    std::vector<size_t> sequence;
    std::vector<std::vector<size_t>> departure_times;
};

struct Individual {
    Chromosome chrom;
    Solution sol;
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
