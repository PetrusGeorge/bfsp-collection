#include "IG_IJ.h"

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
#include "constructions/NEH.h"
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

IG_IJ::IG_IJ(Instance instance, Parameters params)
    : m_instance(std::move(instance)), m_instance_reverse(m_instance.create_reverse_instance()),
      m_params(std::move(params)) {
        m_T = m_params.tP() * m_instance.all_processing_times_sum() / 10*m_instance.num_jobs()*m_instance.num_machines();
        }


double IG_IJ::acceptance_criterion(Solution pi_0, Solution pi_2) {
    return (pi_0.cost - pi_2.cost)/m_T;
}


void IG_IJ::BestSwap(Solution &solution) { // NOLINT

    for (size_t i = 0; i < solution.sequence.size() - 1; i++) {
        // Set correct departure times matrix to use partial recalculate solution
        core::recalculate_solution(m_instance, solution);

        size_t best_j = 0;
        size_t best_cost = solution.cost;

        for (size_t j = i + 1; j < solution.sequence.size(); j++) {
            // Apply move
            std::swap(solution.sequence[i], solution.sequence[j]);

            if (i == 0) {
                // If the first job is swapped it needs a full recalculation
                core::recalculate_solution(m_instance, solution);
            } else {
                core::partial_recalculate_solution(m_instance, solution, i);
            }

            if (solution.cost <= best_cost) {
                best_cost = solution.cost;
                best_j = j;
            }
            std::swap(solution.sequence[i], solution.sequence[j]);
        }
        if (best_j != 0) {
            std::swap(solution.sequence[i], solution.sequence[best_j]);
            solution.cost = best_cost;
        }
        core::recalculate_solution(m_instance, solution);
    }
}


Solution IG_IJ::solve() {

    VERBOSE(m_params.verbose()) << "Initial solution started\n";
    PF_NEH pf_neh(m_instance);
    Solution current = pf_neh.solve(20);
    Solution best = current;
    Solution incumbent = current;
    std::vector<size_t> reference = current.sequence;

    VERBOSE(m_params.verbose()) << "Initial solution finished, solution obtained:\n";
    VERBOSE(m_params.verbose()) << current;

    // Set time limit to parameter or a default calculation
    size_t time_limit = 0;
    size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    if (auto tl = m_params.tl()) {
        time_limit = *tl;
    } else {
        time_limit = (m_params.ro() * mxn) / 1000;
    }

    VERBOSE(m_params.verbose()) << "Time limit: " << time_limit << "s\n";
    NEH neh(m_instance);

    double r = (double)(rand() / RAND_MAX);
    double jP = m_params.jP();

    while (true) {
        std::vector<size_t> removed = destroy(incumbent);
        neh.second_step(std::move(removed), incumbent); // Construct phase

        if(r < jP)
            BestSwap(incumbent);
        else
            rls(incumbent, reference, m_instance);

        
        //  Program should not accept any solution if the time is out
        if (uptime() > time_limit) {
            break;
        }
        if (incumbent.cost < current.cost) {
            current = incumbent;

            if(incumbent.cost < best.cost){
                VERBOSE(m_params.verbose()) << "Found a new best\n";
                VERBOSE(m_params.verbose()) << incumbent;
                best = current = std::move(incumbent);
            }
        }
        // If the solution is worse than the best it's accepted 50% of the times
        else if (r < acceptance_criterion(incumbent, current)) {
            current = std::move(incumbent);
        }

        incumbent = current;
    }

    return best;
}

std::vector<size_t> IG_IJ::destroy(Solution &s) {

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