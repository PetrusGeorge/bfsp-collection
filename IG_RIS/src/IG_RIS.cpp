#include "IG_RIS.h"

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
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - global_start_time);
    return duration.count();
}
} // namespace

IG_RIS::IG_RIS(Instance instance, Parameters params)
    : m_instance(std::move(instance)), m_instance_reverse(m_instance.create_reverse_instance()),
      m_params(std::move(params)) {
        m_T = m_params.tP() * m_instance.all_processing_times_sum() / (10*m_instance.num_jobs()*m_instance.num_machines());
      }


double IG_RIS::acceptance_criterion(Solution& pi_0, Solution& pi_2) {
    double delta = pi_0.cost - pi_2.cost;
    
    return std::exp(-delta/m_T);
}

Solution IG_RIS::solve() {

    VERBOSE(m_params.verbose()) << "Initial solution started\n";

    size_t n = m_instance.num_jobs();
    size_t lambda = n > 200 ? 20 : n; // setting PF-NEH parameter
    PF_NEH pf_neh(m_instance);
    Solution current = pf_neh.solve(lambda);
    Solution best = current;
    Solution incumbent = current;
    std::vector<size_t> reference = current.sequence;

    VERBOSE(m_params.verbose()) << "Initial solution finished, solution obtained:\n";
    VERBOSE(m_params.verbose()) << current;

    // Set time limit to parameter or a default calculation
    size_t time_limit = 0;
    const size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    if (auto tl = m_params.tl()) {
        time_limit = *tl;
    } else {
        time_limit = (m_params.ro() * mxn);
    }

    std::vector<size_t> ro;
    if (m_params.becnhmark()) {
        time_limit = (100 * mxn); // RO == 100
        ro = {90, 60, 30};
    }

    VERBOSE(m_params.verbose()) << "Time limit: " << time_limit << "s\n";
    NEH neh(m_instance);

    while (true) {
        // DestructConstruct Perturbation
        std::vector<size_t> removed = destroy(incumbent);
        neh.second_step(std::move(removed), incumbent); // Construct phase

        // local search
        rls(incumbent, reference, m_instance);
        
        if (!ro.empty() && uptime() >= (ro.back() * mxn)) {
            std::cout << best.cost << '\n';
            ro.pop_back();
        }

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
        else if (RNG::instance().generate_real_number(0, 1) < acceptance_criterion(incumbent, current)) {
            current = std::move(incumbent);
        }

        incumbent = current;
    }

    return best;
}

std::vector<size_t> IG_RIS::destroy(Solution &s) {

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
