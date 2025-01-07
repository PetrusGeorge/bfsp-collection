#ifndef SOLUTION_H
#define SOLUTION_H

#include <limits>
#include <vector>

struct Solution {
    size_t cost = std::numeric_limits<size_t>::max();
    std::vector<size_t> sequence;
    std::vector<std::vector<size_t>> departure_times;
};

#endif
