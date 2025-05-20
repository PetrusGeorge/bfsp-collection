#include "SVNS_D.h"
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
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - global_start_time);
    return static_cast<double>(duration.count());
}
} // namespace

SVNS_D::SVNS_D(Instance &instance, Parameters &params) : m_instance(instance), m_params(params) {}

Solution SVNS_D::PW_PWE2() {
    Solution solution;

    PW pw(m_instance);
    NEH neh(m_instance);

    Instance reversed_instance = m_instance.create_reverse_instance();

    NEH neh_reversed(reversed_instance);

    Solution pw_solution = pw.solve();
    core::recalculate_solution(m_instance, pw_solution);

    Solution pwe_solution = neh.solve(pw_solution.sequence);
    core::recalculate_solution(m_instance, pwe_solution);

    Solution pwe_reversed_solution = neh_reversed.solve(pw_solution.sequence);
    core::recalculate_solution(m_instance, pwe_reversed_solution);

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
    return solution;
}

bool SVNS_D::LS1_D_swap(Solution &solution, std::vector<size_t> &reference) { // NOLINT
    core::recalculate_solution(m_instance, solution);
    size_t original_cost = solution.cost;

    for (size_t i = 0; i < solution.sequence.size() - 1; i++) {
        // Set correct departure times matrix to use partial recalculate solution
        core::recalculate_solution(m_instance, solution);

        size_t index = reference[i];
        size_t best_j = 0;
        size_t best_cost = solution.cost;

        for (size_t j = index + 1; j < solution.sequence.size(); j++) {
            // Apply move
            std::swap(solution.sequence[index], solution.sequence[j]);

            core::recalculate_solution(m_instance, solution);

            if (solution.cost <= best_cost) {
                best_cost = solution.cost;
                best_j = j;
            }
            std::swap(solution.sequence[index], solution.sequence[j]);

            core::recalculate_solution(m_instance, solution);
        }
        if (best_j != 0) {
            std::swap(solution.sequence[index], solution.sequence[best_j]);
            solution.cost = best_cost;
        }
        core::recalculate_solution(m_instance, solution);
    }
    return solution.cost < original_cost;
}

bool SVNS_D::LS2_D_insertion(Solution &solution, std::vector<size_t> &reference) { // NOLINT
    core::recalculate_solution(m_instance, solution);
    size_t original_cost = solution.cost;

    for (size_t i = 0; i < solution.sequence.size(); i++) {
        core::recalculate_solution(m_instance, solution);

        size_t index = reference[i];
        size_t best_j = 0;
        size_t best_cost = solution.cost;

        for (size_t j = 0; j < solution.sequence.size(); j++) {
            // Apply move
            apply_insertion(solution, (long)index, (long)j);

            core::recalculate_solution(m_instance, solution);

            if (solution.cost <= best_cost) {
                best_cost = solution.cost;
                best_j = j;
            }
            // undo move
            apply_insertion(solution, (long)j, (long)index);

            core::recalculate_solution(m_instance, solution);
        }
        if (best_j != index) {
            apply_insertion(solution, (long)index, (long)best_j);
            solution.cost = best_cost;
        }
        core::recalculate_solution(m_instance, solution);
    }
    return solution.cost < original_cost;
}

void SVNS_D::apply_insertion(Solution &solution, const long from, const long to) {
    if (from < to) {
        std::rotate(solution.sequence.begin() + from, solution.sequence.begin() + from + 1,
                    solution.sequence.begin() + to + 1);
    } else {
        std::rotate(solution.sequence.begin() + to, solution.sequence.begin() + from,
                    solution.sequence.begin() + from + 1);
    }
}

void SVNS_D::pertubation(Solution &solution) {
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

Solution SVNS_D::solve() {
    std::optional<size_t> seed = m_params.seed();

    if (seed.has_value()) {
        RNG::instance().set_seed(seed.value());
    }
    VERBOSE(m_params.verbose()) << "Initial solution started\n";

    Solution current = PW_PWE2();
    Solution global_best = current;

    VERBOSE(m_params.verbose()) << "Initial solution finished, solution obtained:\n";
    VERBOSE(m_params.verbose()) << current;

    const double time_limit = 60;
    VERBOSE(m_params.verbose()) << "Time limit: " << time_limit << " seconds\n";

    while (true) {
        Solution local_best = current;
        size_t nml1 = 0;
        size_t indmet = 0;

        if (RNG::instance().generate(0, 1) < m_params.beta()) {
            indmet = 0;
        } else {
            indmet = 1;
        }

        while (true) {
            ++nml1;
            Solution candidate = current;
            size_t og_cost = candidate.cost;

            if (indmet == 0) {
                std::vector<size_t> reference(m_instance.num_jobs());

                for (size_t i = 0; i < m_instance.num_jobs(); ++i) {
                    reference[i] = i;
                }
                while (uptime() <= time_limit) {
                    std::shuffle(reference.begin(), reference.end(), RNG::instance().gen());

                    if (LS1_D_swap(candidate, reference)) {
                        VERBOSE(m_params.verbose()) << "LS1_D Improved! " << og_cost << " " << candidate.cost << "\n";
                        break;
                    }
                }
            } else {
                std::vector<size_t> reference(m_instance.num_jobs());

                for (size_t i = 0; i < m_instance.num_jobs(); ++i) {
                    reference[i] = i;
                }
                while (uptime() <= time_limit) {
                    std::shuffle(reference.begin(), reference.end(), RNG::instance().gen());

                    if (LS2_D_insertion(candidate, reference)) {
                        VERBOSE(m_params.verbose()) << "LS2_D Improved! " << og_cost << " " << candidate.cost << "\n";
                        break;
                    }
                }
            }
            if (candidate.cost < local_best.cost || nml1 == 1) {
                current = candidate;
                local_best = candidate;
                indmet = 1 - indmet;
            } else {
                break;
            }
        };
        if (local_best.cost < global_best.cost) {
            global_best = local_best;
        }
        if (global_best.cost < local_best.cost and RNG::instance().generate(0, 1) < m_params.alpha()) {
            local_best = global_best;
        }
        pertubation(local_best);

        if (local_best.cost < global_best.cost) {
            global_best = local_best;
        }
        if (uptime() > time_limit) {
            VERBOSE(m_params.verbose()) << "Time limit reached!\n";
            break;
        }
    }

    return global_best;
}
