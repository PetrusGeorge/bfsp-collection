#include "SVNS_D.h"

#include "Core.h"
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

void SVNS_D::LS1_D_swap(Solution &solution, std::vector<size_t> &reference) { // NOLINT

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
        }
        core::recalculate_solution(m_instance, solution);
    }
}

void SVNS_D::LS1_D(Solution &solution) { // NOLINT
    std::vector<size_t> reference(m_instance.num_jobs());
    std::iota(reference.begin(), reference.end(), 1);

    const size_t original_cost = solution.cost;

    while (true) {
        std::shuffle(reference.begin(), reference.end(), RNG::instance().gen());

        LS1_D_swap(solution, reference);

        if (solution.cost < original_cost) {
            break;
        }
    }
}

void SVNS_D::LS2_D_insertion(Solution &solution, std::vector<size_t> &reference) { // NOLINT
    for (size_t i = 0; i < solution.sequence.size(); i++) {
        size_t index = reference[i];
        size_t best_j = 0;
        size_t best_cost = solution.cost;

        for (size_t j = 0; j < solution.sequence.size(); j++) {
            if (index == j) {
                continue;
            }
            // Apply move
            apply_insertion(solution, (long)index, (long)j);

            if (solution.cost <= best_cost) {
                best_cost = solution.cost;
                best_j = j;
            }
            // undo move
            apply_insertion(solution, (long)j, (long)index);
        }
        if (best_j != index) {
            apply_insertion(solution, (long)index, (long)best_j);
            solution.cost = best_cost;
        }
        core::recalculate_solution(m_instance, solution);
    }
}

void SVNS_D::apply_insertion(Solution &solution, const long from, const long to) {
    if (from < to) {
        std::rotate(solution.sequence.begin() + from, solution.sequence.begin() + from + 1,
                    solution.sequence.begin() + to + 1);
    } else {
        std::rotate(solution.sequence.begin() + to, solution.sequence.begin() + from,
                    solution.sequence.begin() + from + 1);
    }
    core::recalculate_solution(m_instance, solution);
}

void SVNS_D::LS2_D(Solution &solution) { // NOLINT
    std::vector<size_t> reference(m_instance.num_jobs());

    for (size_t i = 0; i < solution.sequence.size(); ++i) {
        reference[i] = i;
    }

    const size_t original_cost = solution.cost;

    while (true) {
        std::shuffle(reference.begin(), reference.end(), RNG::instance().gen());

        LS2_D_insertion(solution, reference);

        if (solution.cost < original_cost) {
            break;
        }
    }
}

Solution SVNS_D::solve() {
    std::optional<size_t> seed = m_parameters.seed();

    if (seed.has_value()) {
        RNG::instance().set_seed(seed.value());
    }

    Solution local_best = PW_PWE2();

    Solution global_best = local_best;

    while (true) {
        size_t nml1 = 0;
        size_t sampled_local_search = 0;

        if (RNG::instance().generate(0, 1) < m_parameters.beta()) {
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
        if (global_best.cost < local_best.cost and RNG::instance().generate(0, 1) < m_parameters.alpha()) {
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
