#include <algorithm>
#include <cstdio>
#include <iostream>
#include <limits>
#include <random>

#include "Core.h"
#include "DIWO.h"
#include "Instance.h"
#include "NEH.h"
#include "PF_NEH.h"
#include "RLS.h"
#include "Solution.h"

static const double EPSILON = std::numeric_limits<double>::min();

namespace {
size_t uptime() {
    static const auto global_start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - global_start_time);
    return duration.count();
}
} // namespace

void Population::add_solution(Solution solution) {
    const size_t sol_cost = solution.cost;
    solutions.emplace_back(std::move(solution));

    if (solutions.size() == 1) {
        best_solution_idx = 0;
        worst_solution_idx = 0;
        return;
    }

    if (sol_cost < solutions[best_solution_idx].cost) {
        best_solution_idx = solutions.size() - 1;
    } else if (sol_cost > solutions[worst_solution_idx].cost) {
        worst_solution_idx = solutions.size() - 1;
    }
}

bool Population::has_solution(const Solution &solution) const {
    for (const auto &sol : solutions) {
        for (size_t i = 0; i < sol.sequence.size(); ++i) {
            if (solution.sequence[i] != sol.sequence[i]) {
                break;
            }
            if (i == sol.sequence.size() - 1) {
                return true;
            }
        }
    }

    return false;
}

void Population::calculate_seeds(size_t s_min, size_t s_max) {
    seeds.resize(solutions.size());

    const auto t_worst = static_cast<double>(solutions[worst_solution_idx].cost);
    const auto t_best = static_cast<double>(solutions[best_solution_idx].cost);
    const auto multiplier = static_cast<double>(s_max - s_min);

    for (size_t i = 0; i < solutions.size(); ++i) {
        const double div = (t_worst - static_cast<double>(solutions[i].cost) + EPSILON) /
                           (static_cast<double>(t_worst - t_best) + EPSILON);
        seeds[i] = static_cast<size_t>(std::floor(div * multiplier)) + s_min;
    }
}

Population DIWO::population_init() {
    Solution best;

    const std::vector<size_t> stpt = core::stpt_sort(m_instance);
    const size_t lambda = m_instance.num_jobs() >= 25 ? 25 : m_instance.num_jobs();

    for (size_t i = 0; i < 5; ++i) {
        PF_NEH pf_neh(m_instance);

        const Solution sol = pf_neh.solve(lambda, i);

        if (sol.cost < best.cost) {
            best = sol;
        }
    }

    // Calling it to set the departure times
    core::recalculate_solution(m_instance, best);

    Population pop_unordered;
    pop_unordered.add_solution(std::move(best));

    Solution tmp;

    // N_0 is P_max
    while (pop_unordered.solutions.size() < m_params.p_max()) {
        tmp.sequence = std::vector<size_t>(m_instance.num_jobs());
        std::iota(tmp.sequence.begin(), tmp.sequence.end(), 0);
        std::shuffle(tmp.sequence.begin(), tmp.sequence.end(), m_rng);

        if (!pop_unordered.has_solution(tmp)) {
            core::recalculate_solution(m_instance, tmp);
            pop_unordered.add_solution(std::move(tmp));
        }
    }

    // Sort to get median for spatial dispersal
    std::sort(pop_unordered.solutions.begin(), pop_unordered.solutions.end(),
              [](const Solution &a, const Solution &b) { return a.cost < b.cost; });
    Population pop;
    for (auto &sol : pop_unordered.solutions) {
        pop.add_solution(std::move(sol));
    }

    return pop;
}

size_t DIWO::get_solution_d(const double deviation) {

    size_t d = std::floor(std::abs(std::normal_distribution{0.0, std::pow(deviation, 2)}(m_rng)));

    if (d > m_instance.num_jobs() / 2 || d < m_params.sigma_min()) {
        d = std::floor(static_cast<double>(m_params.sigma_min()) +
                       (RNG::instance().generate_real_number(0.0, 1.0) *
                        static_cast<double>(m_params.sigma_max() - m_params.sigma_min())));
    }

    return d;
}

Population DIWO::spatial_dispersal(const Population &pop) {
    Population new_pop;
    NEH neh(m_instance);

    const size_t max_time = m_params.ro() * m_instance.num_jobs() * m_instance.num_machines();

    double deviation = ((1 - static_cast<double>(uptime()) / static_cast<double>(max_time)) *
                        static_cast<double>(m_params.sigma_max() - m_params.sigma_min())) +
                       static_cast<double>(m_params.sigma_min());

    for (size_t i = 0; i < pop.solutions.size(); ++i) {
        const auto &sol = pop.solutions[i];

        const size_t half_size = pop.solutions.size() / 2;
        const double median =
            pop.solutions.size() % 2 == 0
                ? static_cast<double>(pop.solutions[half_size - 1].cost + pop.solutions[half_size].cost) / 2.0
                : static_cast<double>(pop.solutions[half_size].cost);

        if (static_cast<double>(sol.cost) > median) {
            deviation *= ((static_cast<double>(sol.cost) - median) /
                          (static_cast<double>(pop.solutions[pop.worst_solution_idx].cost) - median)) *
                             0.5 + 1;
        }
        for (size_t j = 0; j < static_cast<size_t>(pop.seeds[i]); ++j) {

            const size_t d = get_solution_d(deviation);

            Solution sol_copy;
            sol_copy.sequence = sol.sequence;
            std::vector<size_t> pi_r;

            for (size_t k = 0; k < d; ++k) {
                const long idx =
                    RNG::instance().generate(static_cast<long>(0), static_cast<long>(sol_copy.sequence.size()) - 1);
                pi_r.push_back(sol_copy.sequence[idx]);

                sol_copy.sequence.erase(sol_copy.sequence.begin() + idx);
            }
            core::recalculate_solution(m_instance, sol_copy);

            std::sort(pi_r.begin(), pi_r.end(), [this](const size_t a, const size_t b) {
                return m_instance.processing_times_sum()[a] < m_instance.processing_times_sum()[b];
            });

            neh.second_step(std::move(pi_r), sol_copy);

            new_pop.add_solution(std::move(sol_copy));
        }
    }
    return new_pop;
}

void DIWO::local_search(Population &pop) {
    for (size_t i = 0; i < pop.solutions.size(); i++) {
        if (RNG::instance().generate_real_number(0.0, 1.0) < m_params.pls()) {
            continue;
        }
        std::vector<size_t> ref = pop.solutions[pop.best_solution_idx].sequence;
        for (size_t j = 0; j < 3; j++) {
            rls_grabowski(pop.solutions[i], ref, m_instance);
            // Update best solution if it's found by rls
            if (pop.solutions[i].cost < pop.solutions[pop.best_solution_idx].cost) {
                pop.best_solution_idx = i;
            }
            std::shuffle(ref.begin(), ref.end(), m_rng);
        }
    }
}

Population DIWO::competitive_exclusion(Population pop, Population new_pop) const {
    Population competitive_pop;

    pop.solutions.reserve(pop.solutions.size() + new_pop.solutions.size());
    pop.solutions.insert(pop.solutions.end(), std::make_move_iterator(new_pop.solutions.begin()),
                         std::make_move_iterator(new_pop.solutions.end()));

    std::sort(pop.solutions.begin(), pop.solutions.end(),
              [](const Solution &a, const Solution &b) { return a.cost < b.cost; });

    competitive_pop.add_solution(std::move(pop.solutions[0]));

    for (size_t i = 1; i < pop.solutions.size(); i++) {
        if (competitive_pop.solutions.size() >= m_params.p_max()) {
            break;
        }

        if (!competitive_pop.has_solution(pop.solutions[i])) {
            competitive_pop.add_solution(std::move(pop.solutions[i]));
        }
    }

    return competitive_pop;
}

Solution DIWO::solve() {
    const size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    size_t time_limit = m_params.ro() * m_instance.num_jobs() * m_instance.num_machines();
    std::vector<size_t> ro;
    if (m_params.benchmark()) {
        time_limit = 100 * mxn; // RO == 100
        ro = {90, 60, 30};
    }
    auto pop = population_init();

    Solution best = pop.solutions[pop.best_solution_idx];

    while (true) {

        if (!ro.empty() && uptime() >= ro.back() * mxn) {
            std::cout << best.cost << '\n';
            ro.pop_back();
        }

        if (uptime() > time_limit) {
            break;
        }

        best = pop.solutions[pop.best_solution_idx];
        pop.calculate_seeds(m_params.s_min(), m_params.s_max());

        Population new_pop = spatial_dispersal(pop);
        local_search(new_pop);

        pop = competitive_exclusion(std::move(pop), std::move(new_pop));
    }

    return best;
}
