#include "IG.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <vector>

#include "Core.h"
#include "Instance.h"
#include "Log.h"
#include "MinMax.h"
#include "NEH.h"
#include "RNG.h"
#include "Solution.h"

namespace {
size_t uptime() {
    static const auto global_start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - global_start_time);
    return duration.count();
}
} // namespace

IG::IG(Instance instance, Parameters params)
    : m_instance(std::move(instance)), m_instance_reverse(m_instance.create_reverse_instance()),
      m_params(std::move(params)) {}

Solution IG::initial_solution() {
    Solution best;

    // Direct MME
    MinMax mm(m_instance);
    NEH neh(m_instance);
    Solution normal = neh.solve(mm.solve().sequence);
    // core::recalculate_solution(normal);

    // Reverse MME
    MinMax mmr(m_instance);
    NEH nehr(m_instance);
    Solution jobs_reversed = nehr.solve(mmr.solve().sequence);
    std::reverse(jobs_reversed.sequence.begin(), jobs_reversed.sequence.end());
    core::recalculate_solution(m_instance, jobs_reversed);

    best = normal.cost < jobs_reversed.cost ? std::move(normal) : std::move(jobs_reversed);

    return best;
}

Solution IG::solve() {

    VERBOSE(m_params.verbose()) << "Initial solution started\n";
    Solution current = initial_solution();
    Solution best = current;
    VERBOSE(m_params.verbose()) << "Initial solution finished, solution obtained:\n";
    VERBOSE(m_params.verbose()) << current;

    const size_t time_limit = (m_params.ro() * m_instance.num_jobs() * m_instance.num_machines())/1000;
    std::cout << "Time limit: " << time_limit << "s\n";
    NEH neh(m_instance);

    while (true) {
        Solution incumbent = local_search(current);

        //  Program should not accept any solution if the time is out
        if (uptime() > time_limit) {
            break;
        }
        if (incumbent.cost < best.cost) {
            VERBOSE(m_params.verbose()) << "Found a new best\n";
            VERBOSE(m_params.verbose()) << incumbent;
            best = current = std::move(incumbent);
        }
        // If the solution is worse than the best it's accepted 50% of the times
        else if (RNG::instance().generate(0, 1) == 1) {
            current = std::move(incumbent);
        }

        std::vector<size_t> removed = destroy(current);
        neh.second_step(std::move(removed), current); // Construct phase
    }

    return best;
}

bool IG::swap_first_improvement(Solution &s) {
    Solution copy = s;

    for (size_t i = 0; i < s.sequence.size() - 1; i++) {
        for (size_t j = i + 1; j < s.sequence.size(); j++) {
            // Apply move
            std::swap(copy.sequence[i], copy.sequence[j]);
            if ( i > 2 ) {
                core::partial_recalculate_solution(m_instance, copy, i-1);
            } else {
                core::recalculate_solution(m_instance, copy);
            }

            if (copy.cost < s.cost) {
                s = std::move(copy);
                return true;
            }

            // Undo move
            std::swap(copy.sequence[i], copy.sequence[j]);
        }
    }
    return false;
}

Solution IG::local_search(Solution s) {
    while (true) {
        if (!swap_first_improvement(s)) {
            break;
        }
    }
    return s;
}

std::vector<size_t> IG::destroy(Solution &s) {

    // This mostly avoids crashes on really small toy instances
    const size_t destroy_size = std::min(s.sequence.size() - 1, m_params.d());

    std::vector<size_t> removed;
    removed.reserve(destroy_size);
    // Random remove d nodes
    for (size_t i = 0; i < destroy_size; i++) {
        const long chose = RNG::instance().generate<long>(0, ((long)s.sequence.size()) - 1);
        removed.push_back(s.sequence[chose]);
        s.sequence.erase(s.sequence.begin() + chose);
    }

    return removed;
}
