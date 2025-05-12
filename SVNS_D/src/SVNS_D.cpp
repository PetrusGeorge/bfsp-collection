#include "SVNS_D.h"

#include "Core.h"
#include "Solution.h"
#include "constructions/NEH.h"
#include "constructions/PW.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <numeric>
#include <optional>
#include <vector>

namespace {
double uptime() {
    static const auto global_start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - global_start_time);
    return static_cast<double>(duration.count());
}
} // namespace

SVNS_D::SVNS_D(Instance &instance, Parameters &params) : m_instance(instance), m_parameters(params) {}

Solution SVNS_D::PW_PWE2() {
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
    return solution;
}

bool SVNS_D::LS1_D_swap(Solution &solution, std::vector<size_t> &reference) {
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

            if (index == 0) {
                // If the first job is swapped it needs a full recalculation
                core::recalculate_solution(m_instance, solution);
            } else {
                core::partial_recalculate_solution(m_instance, solution, index);
            }

            if (solution.cost <= best_cost) {
                best_cost = solution.cost;
                best_j = j;
            }
            std::swap(solution.sequence[index], solution.sequence[j]);
        }
        if (best_j != 0) {
            std::swap(solution.sequence[index], solution.sequence[best_j]);
            solution.cost = best_cost;
            // If a swap happens it'll make the departure times matrix inconsistent
            // make sure that you recalculate it if needed after the usage of this function
        }
    }

    return solution.cost < original_cost;
}

void SVNS_D::LS1_D(Solution &solution) { // NOLINT
    std::vector<size_t> reference(m_instance.num_jobs());
    std::iota(reference.begin(), reference.end(), 1);

    while (true) {
        std::shuffle(reference.begin(), reference.end(), m_rng);

        LS1_D_swap(solution, reference);
    }
}

Solution SVNS_D::solve() {
    std::optional<size_t> seed = m_parameters.seed();

    if (seed.has_value()) {
        m_rng.set_seed(seed.value());
    }

    Solution local_best = PW_PWE2();

    Solution global_best = local_best;

    while (true) {
        size_t nml1 = 0;
        size_t sampled_local_search = 0;

        if (m_rng.generate<double>(0, 1) < m_parameters.beta()) {
            sampled_local_search = 0;
        } else {
            sampled_local_search = 1;
        }

        while (true) {
            ++nml1;

            Solution current = local_best; // NOLINT

            if (sampled_local_search == 0) {
                LS1_D(current);
            } else {
                /*LS2(current); */
            }
            if (local_best.cost < current.cost or nml1 == 1) {
                local_best = current;
                sampled_local_search = 1 - sampled_local_search;
            } else {
                break;
            }
        };
        if (local_best.cost < global_best.cost) {
            global_best = local_best;
        }
        if (global_best.cost < local_best.cost and m_rng.generate<double>(0, 1) < m_parameters.alpha()) {
            local_best = global_best;
        }
        /*Solution desconstructed = desconstruct(local_best);*/
        /*local_best = construct(desconstructed);*/
        if (local_best.cost < global_best.cost) {
            global_best = local_best;
        }
    }

    return global_best;
}
