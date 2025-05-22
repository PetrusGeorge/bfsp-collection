#include "MFFO.h"
#include "Core.h"
#include "Instance.h"
#include "NEH.h"
#include "Parameters.h"
#include "RLS.h"
#include "RNG.h"
#include "MinMax.h"
#include "Solution.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <vector>

namespace {
size_t uptime() {
    static const auto global_start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - global_start_time);
    return duration.count();
}
} 

Solution MFFO::solve() {
    uptime(); // Start counting time
    const size_t n_jobs = m_instance.num_jobs();
    std::vector<Solution> population(m_param.ps());

    // Population initialization
    NEH neh(m_instance);
    MinMax min_max(m_instance);
    population[0] = neh.solve(min_max.solve().sequence);
    for (size_t i = 1; i < m_param.ps(); i++) {
        population[i].sequence.resize(n_jobs);
        std::iota(population[i].sequence.begin(), population[i].sequence.end(), 0);
        std::shuffle(population[i].sequence.begin(), population[i].sequence.end(), RNG::instance().gen());

        core::recalculate_solution(m_instance, population[i]);
    }

    Solution best = population[0];

    // Smell-based search
    const size_t mxn = m_instance.num_jobs() * m_instance.num_machines();

    size_t time_limit = m_param.ro() * m_instance.num_jobs() * m_instance.num_machines();
    std::vector<size_t> ro;

    if (m_param.benchmark()) {
        time_limit = 100 * mxn; // RO == 100
        ro = {90, 60, 30};
    }

    size_t i = 0;
    while (true) {

        Solution s1 = MFFO::neighbourhood_search(best);

        // Determine whether to apply RLS based on a probability
        if (RNG::instance().generate_real_number(0.0, 1.0) < m_param.pls()) {
            rls(s1, best.sequence, m_instance);
        }

        if (!ro.empty() && uptime() >= ro.back() * mxn) {
            std::cout << best.cost << '\n';
            ro.pop_back();
        }

        if (uptime() > time_limit) {
            break;
        }

        // Update population and best solution if s1 is better
        if (s1.cost < population[i].cost) {
            population[i] = s1;
            if (s1.cost < best.cost) {
                best = s1;
            }
        }

        i = (i + 1) % m_param.ps();
    }

    return best;
}

inline void MFFO::neighbourhood_insertion_first(Solution &s) {
    const int p2 = RNG::instance().generate(1, (int)s.sequence.size() - 1);
    const int p1 = RNG::instance().generate(0, p2 - 1);

    std::rotate(s.sequence.begin() + p1, s.sequence.begin() + p1 + 1, s.sequence.begin() + p2 + 1);
}

inline void MFFO::neighbourhood_insertion_back(Solution &s) {
    const int p2 = RNG::instance().generate(1, (int)s.sequence.size() - 1);
    const int p1 = RNG::instance().generate(0, p2 - 1);

    std::rotate(s.sequence.begin() + p1, s.sequence.begin() + p2, s.sequence.begin() + p2 + 1);
}

inline void MFFO::neighbourhood_swap(Solution &s) {
    const size_t p1 = RNG::instance().generate(0, (int)s.sequence.size() - 1);
    size_t p2 = RNG::instance().generate(0, (int)s.sequence.size() - 1);

    while (p1 == p2) {
        p2 = RNG::instance().generate(0, (int)s.sequence.size() - 1);
    }

    std::swap(s.sequence[p1], s.sequence[p2]);
}

Solution MFFO::neighbourhood_search(Solution s) {
    switch (RNG::instance().generate(0, 3)) {
    case 0:
        if (RNG::instance().generate(0, 1) != 0) {
            MFFO::neighbourhood_insertion_first(s);
        } else {
            MFFO::neighbourhood_insertion_back(s);
        }
        break;
    case 1:
        MFFO::neighbourhood_swap(s);
        break;
    case 2:
        if (RNG::instance().generate(0, 1) != 0) {
            MFFO::neighbourhood_insertion_first(s);
            if (RNG::instance().generate(0, 1) != 0) {
                MFFO::neighbourhood_insertion_first(s);
            } else {
                MFFO::neighbourhood_insertion_back(s);
            }
        } else {
            MFFO::neighbourhood_insertion_back(s);
            if (RNG::instance().generate(0, 1) != 0) {
                MFFO::neighbourhood_insertion_first(s);
            } else {
                MFFO::neighbourhood_insertion_back(s);
            }
        }
        break;
    case 3:
        MFFO::neighbourhood_swap(s);
        MFFO::neighbourhood_swap(s);
        break;
    }
    return s;
}
