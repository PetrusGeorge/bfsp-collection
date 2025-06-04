#include "SVNS_S.h"
#include "Core.h"
#include "Log.h"
#include "RNG.h"
#include "Solution.h"

#include "constructions/NEH.h"
#include "constructions/PW.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <optional>
#include <vector>

namespace {
double uptime() {
    static const auto global_start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - global_start_time);
    return static_cast<double>(duration.count());
}
} // namespace

SVNS_S::SVNS_S(Instance &instance, Parameters &params) : m_instance(instance), m_params(params) {}

Solution SVNS_S::PW_PWE2() {
    Solution solution;

    PW pw(m_instance);
    NEH neh(m_instance);

    Instance reversed_instance = m_instance.create_reverse_instance();

    NEH neh_reversed(reversed_instance);

    Solution pw_solution = pw.solve();

    Solution pwe_solution = neh.solve(pw_solution.sequence);

    Solution pwe_reversed_solution = neh_reversed.solve(pw_solution.sequence);

    Solution pwe2_solution;

    if (pwe_solution.cost < pwe_reversed_solution.cost) {
        pwe2_solution = pwe_solution;
    } else {
        pwe2_solution = pwe_reversed_solution;
    }

    if (pwe2_solution.cost < pw_solution.cost) {
        solution = pwe2_solution;
    } else {
        solution = pw_solution;
    }

    core::recalculate_solution(m_instance, solution);
    return solution;
}

bool SVNS_S::LS1_S_swap(Solution &solution, std::vector<size_t> &reference) { // NOLINT
    size_t original_cost = solution.cost;

    for (size_t i = 0; i < solution.sequence.size() - 1; i++) {
        size_t index = reference[i];
        size_t best_j = 0;
        size_t best_cost = solution.cost;

        for (size_t j = index + 1; j < solution.sequence.size(); j++) {
            size_t cost_temp = solution.cost;
            // Apply move
            std::swap(solution.sequence[index], solution.sequence[j]);

            core::recalculate_solution(m_instance, solution);

            if (solution.cost <= best_cost) {
                best_cost = solution.cost;
                best_j = j;
            }
            std::swap(solution.sequence[index], solution.sequence[j]);
            solution.cost = cost_temp;
        }
        if (best_cost < solution.cost) {
            std::swap(solution.sequence[index], solution.sequence[best_j]);
            solution.cost = best_cost;
        }
    }
    
    return solution.cost < original_cost;
}

bool SVNS_S::LS2_S_insertion(Solution &solution, std::vector<size_t> &reference) { // NOLINT
    size_t original_cost = solution.cost;

    for (size_t i = 0; i < solution.sequence.size()-1; i++) {
        size_t index = reference[i];
        size_t best_j = 0;
        size_t best_cost = solution.cost;

        for (size_t j = 0; j < solution.sequence.size(); j++) {
            size_t cost_temp = solution.cost;
            // Apply move
            apply_insertion(solution, (long)index, (long)j);

            core::recalculate_solution(m_instance, solution);

            if (solution.cost <= best_cost) {
                best_cost = solution.cost;
                best_j = j;
            }
            // undo move
            apply_insertion(solution, (long)j, (long)index);
            solution.cost = cost_temp;
        }
        if (best_cost < solution.cost) {
            apply_insertion(solution, (long)index, (long)best_j);
            solution.cost = best_cost;
        }
    }

    return solution.cost < original_cost;
}

void SVNS_S::apply_insertion(Solution &solution, const long from, const long to) {
    if (from < to) {
        std::rotate(solution.sequence.begin() + from, solution.sequence.begin() + from + 1,
                    solution.sequence.begin() + to + 1);
    } else {
        std::rotate(solution.sequence.begin() + to, solution.sequence.begin() + from,
                    solution.sequence.begin() + from + 1);
    }
}

void SVNS_S::pertubation(Solution &solution) {
    std::vector<size_t> removed_jobs(m_params.d());
    for (size_t i = 0; i < m_params.d(); ++i) {
        auto job_to_remove = RNG::instance().generate<size_t>(0, solution.sequence.size() - 1);

        removed_jobs[i] = solution.sequence[job_to_remove];
        solution.sequence.erase(solution.sequence.begin() + (long)job_to_remove);
    }
    NEH neh(m_instance);

    neh.second_step(removed_jobs, solution);

    core::recalculate_solution(m_instance, solution);
}

Solution SVNS_S::solve() {
    std::optional<size_t> seed = m_params.seed();

    if (seed.has_value()) {
        RNG::instance().set_seed(seed.value());
    }
    VERBOSE(m_params.verbose()) << "Initial solution started\n";

    Solution current = PW_PWE2();
    Solution best = current;

    VERBOSE(m_params.verbose()) << "Initial solution finished, solution obtained:\n";
    VERBOSE(m_params.verbose()) << current;

    size_t time_limit = 0;
    const size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    std::vector<size_t> ro;
    if (m_params.benchmark()) {
        time_limit = (100 * mxn); // RO == 100
        ro = {90, 60, 30};
    } else {
        time_limit = m_params.k() * mxn;
    }
    VERBOSE(m_params.verbose()) << "Time limit: " << time_limit/1000 << " seconds\n";

    while (true) {
        size_t counter = 0;
        size_t local_search_type = 0;

        if (RNG::instance().generate(0, 1) < m_params.beta()) {
            local_search_type = 0;
        } else {
            local_search_type = 1;
        }

        while (true) {
            ++counter;
            Solution candidate = current;

            size_t original_cost = current.cost;

            if (local_search_type == 0) {
                std::vector<size_t> reference(m_instance.num_jobs());

                for (size_t i = 0; i < m_instance.num_jobs(); ++i) {
                    reference[i] = i;
                }
                std::shuffle(reference.begin(), reference.end(), RNG::instance().gen());

                LS1_S_swap(candidate, reference);  
            } else {
                std::vector<size_t> reference(m_instance.num_jobs());

                for (size_t i = 0; i < m_instance.num_jobs(); ++i) {
                    reference[i] = i;
                }
                std::shuffle(reference.begin(), reference.end(), RNG::instance().gen());

                LS2_S_insertion(candidate, reference);
            }
    
            if (candidate.cost < original_cost or counter == 1) {
                local_search_type = 1 - local_search_type;
                current = candidate;
                original_cost = candidate.cost;
            } else {
                break;
            }   
    
        };

        if (!ro.empty() && uptime() >= (ro.back() * mxn)) {
            std::cout << best.cost << '\n';
            ro.pop_back();
        }
            
        if (uptime() > time_limit) {
            break;
        }

        if (current.cost < best.cost) {
            best = current;
        }
        if (current.cost > best.cost and RNG::instance().generate(0, 1) < m_params.alpha()) {
            current = best;
        }
        
        pertubation(current);

        if (current.cost < best.cost) {
            best = current;
        }
        if (uptime() > time_limit) {
            VERBOSE(m_params.verbose()) << "Time limit reached!\n";
            break;
        }
    }
    return best;
}
