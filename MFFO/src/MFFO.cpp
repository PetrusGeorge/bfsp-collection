#include "MFFO.h"
#include "Instance.h"
#include "NEH.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

Solution MFFO::solve(MFFO &mffo_instance) {

    const size_t n_jobs = mffo_instance.m_instance.num_jobs();
    const size_t n_machine = mffo_instance.m_instance.num_machines();

    // Population initialization
    NEH neh(mffo_instance.m_instance);
    Solution s1 = neh.solve(MFFO::min_max(mffo_instance.m_instance, mffo_instance.m_param, false));

    // Smell-based search
    auto start_time = high_resolution_clock::now();
    const long max_time = 5 * (long)n_jobs * (long)n_machine;
    while (duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - start_time).count() < max_time) {
        for (size_t i = 0; i < mffo_instance.m_param.ps(); i++) {
            const Solution s2 = MFFO::neighbourhood_search(mffo_instance.m_instance, mffo_instance.m_param, s1);
            // Vision-based search
        }
    }

    return s1;
}

inline void MFFO::neighbourhood_insertion_first(const Instance & /*instance*/, const Parameters & /*param*/, Solution &s) {
    const int p2 = RNG::instance().generate(1, (int)s.sequence.size() - 1);
    const int p1 = RNG::instance().generate(0, p2 - 1);

    std::rotate(s.sequence.begin() + p1, s.sequence.begin() + p1 + 1, s.sequence.begin() + p2 + 1);
}

inline void MFFO::neighbourhood_insertion_back(const Instance & /*instance*/, const Parameters & /*param*/, Solution &s) {
    const int p2 = RNG::instance().generate(1, (int)s.sequence.size() - 1);
    const int p1 = RNG::instance().generate(0, p2 - 1);

    std::rotate(s.sequence.begin() + p1, s.sequence.begin() + p2, s.sequence.begin() + p2 + 1);
}

inline void MFFO::neighbourhood_swap(const Instance & /*instance*/, const Parameters & /*param*/, Solution &s) {
    const size_t p1 = RNG::instance().generate(0, (int)s.sequence.size() - 1);
    size_t p2 = RNG::instance().generate(0, (int)s.sequence.size() - 1);

    while (p1 == p2) {
        p2 = RNG::instance().generate(0, (int)s.sequence.size() - 1);
    }

    std::swap(s.sequence[p1], s.sequence[p2]);
}

Solution MFFO::neighbourhood_search(const Instance &instance, const Parameters &param, Solution &s) {
    switch (RNG::instance().generate(0, 3)) {
    case 0:
        if (RNG::instance().generate(0, 1) != 0) {
            MFFO::neighbourhood_insertion_first(instance, param, s);
        } else {
            MFFO::neighbourhood_insertion_back(instance, param, s);
        }
        break;
    case 1:
        MFFO::neighbourhood_swap(instance, param, s);
        break;
    case 2:
        if (RNG::instance().generate(0, 1) != 0) {
            MFFO::neighbourhood_insertion_first(instance, param, s);
            if (RNG::instance().generate(0, 1) != 0) {
                MFFO::neighbourhood_insertion_first(instance, param, s);
            } else {
                MFFO::neighbourhood_insertion_back(instance, param, s);
            }
        } else {
            MFFO::neighbourhood_insertion_back(instance, param, s);
            if (RNG::instance().generate(0, 1) != 0) {
                MFFO::neighbourhood_insertion_first(instance, param, s);
            } else {
                MFFO::neighbourhood_insertion_back(instance, param, s);
            }
        }
        break;
    case 3:
        MFFO::neighbourhood_swap(instance, param, s);
        MFFO::neighbourhood_swap(instance, param, s);
        break;
    }
    return {};
}

std::vector<size_t> MFFO::min_max(const Instance &instance, const Parameters &param, bool jobs_reversed) {
    std::vector<size_t> s;

    const std::function<long(size_t, size_t)> p = MFFO::get_reversible_matrix(instance, jobs_reversed);

    size_t first_node = 0;

    for (size_t i = 1; i < instance.num_jobs(); i++) {

        const size_t first_machine = 0;

        // Choose the minimum processing on the first machine for the first node
        if (p(i, first_machine) < p(first_node, first_machine)) {
            first_node = i;
        }
    }

    size_t last_node = 0;
    if (first_node == last_node) {
        // Avoid first_node being equal to last_node in case 0 is best in both cases
        last_node = 1;
    }
    for (size_t i = 1; i < instance.num_jobs(); i++) {
        const size_t last_machine = instance.num_machines() - 1;

        // Choose the minimum processing on the last machine for the last node
        if (p(i, last_machine) < p(last_node, last_machine) && i != first_node) {
            last_node = i;
        }
    }

    // Put the remaining nodes in a vector
    std::vector<size_t> cl;
    cl.reserve(instance.num_jobs());
    for (size_t i = 0; i < instance.num_jobs(); i++) {
        if (i != first_node && i != last_node) {
            cl.push_back(i);
        }
    }

    // Expression number 3 from Roconi paper, https://doi.org/10.1016/S0925-5273(03)00065-3
    auto expression = [param, instance, p](size_t c, size_t last) {
        double lhs_value = 0;
        double rhs_value = 0;

        for (size_t j = 0; j < instance.num_machines() - 1; j++) {
            lhs_value += (double)std::abs(p(c, j) - p(last, j + 1));
        }
        lhs_value *= param.alpha();

        for (size_t j = 0; j < instance.num_machines(); j++) {
            rhs_value += (double)p(c, j);
        }
        rhs_value *= 1 - param.alpha();

        return rhs_value + lhs_value;
    };

    s.push_back(first_node);
    // Choose and insert best value node until there is no one left
    while (!cl.empty()) {
        size_t best_node_index = 0;
        double best_node_value = std::numeric_limits<double>::max();
        const size_t last_inserted = s.back();

        for (size_t i = 0; i < cl.size(); i++) {

            const double value = expression(cl[i], last_inserted);
            if (value < best_node_value) {
                best_node_value = value;
                best_node_index = i;
            }
        }

        s.push_back(cl[best_node_index]);
        cl.erase(cl.begin() + (long)best_node_index);
    }

    s.push_back(last_node);
    return s;
}

std::function<long(size_t, size_t)> MFFO::get_reversible_matrix(const Instance &instance, bool reversed) {

    // A c++ hack using lambda functions to call normal matrix view or the jobs_reversed one
    std::function<long(size_t, size_t)> p = [instance](size_t i, size_t j) { return instance.p(i, j); };
    if (reversed) {
        p = [instance](size_t i, size_t j) { return instance.rp(i, j); };
    }

    return p;
}
