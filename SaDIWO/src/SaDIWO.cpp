#include <algorithm>
#include <limits>
#include <numeric>
#include <random>

#include "Clock.h"
#include "Core.h"
#include "Instance.h"
#include "SaDIWO.h"
#include "NEH.h"
#include "Solution.h"
#include "PF_NEH.h"

// static const double EPSILON = 1e-15; // Assumes that the double type is IEEE 754 compliant
static const double EPSILON = std::numeric_limits<double>::min();

void Population::add_solution(const Solution &solution) {
    solutions.push_back(solution);

    if (solutions.size() == 1) {
        best_solution_idx = 0;
        worst_solution_idx = 0;
        return;
    }

    if (solution.cost < solutions[best_solution_idx].cost) {
        best_solution_idx = solutions.size() - 1;
    } else if (solution.cost > solutions[worst_solution_idx].cost) {
        worst_solution_idx = solutions.size() - 1;
    }
}

void Population::add_solutions(const std::vector<Solution> &solutions) {
    for (const auto &sol : solutions) {
        add_solution(sol);
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

void Population::calculate_seeds(const SaDIWOParams &params) {
    seeds.resize(solutions.size());

    const size_t t_worst = solutions[worst_solution_idx].cost;
    const size_t t_best = solutions[best_solution_idx].cost;
    const int multiplier = (params.s_max - params.s_min) + params.s_min;

    for (size_t i = 0; i < solutions.size(); ++i) {
        const double div =
            static_cast<double>(t_worst - solutions[i].cost) / (static_cast<double>(t_worst - t_best) + EPSILON);
        seeds[i] = std::floor(div * multiplier);
    }
}

// void SaDIWO::update_departure_times(std::vector<long> &departures, const size_t curr_job) {
//     long prev_departure = departures[0];
//     for (size_t j = 0; j < m_instance.num_machines() - 1; ++j) {
//         const long tmp = departures[j];
//         departures[j] = std::max(prev_departure + m_instance.p(curr_job, j), departures[j + 1]);
//         prev_departure = tmp;
//     }
//     const size_t last_j = m_instance.num_machines() - 1;
//     departures[last_j] = departures[last_j - 1] + m_instance.p(curr_job, last_j);
// }

long SaDIWO::get_idle_block_sum(const std::vector<long> &departures, const size_t curr_job) {
    long sum = 0;
    long prev_dept = departures[0];

    for (size_t j = 0; j < m_instance.num_machines() - 1; ++j) {
        const long this_dept = std::max(prev_dept + m_instance.p(curr_job, j), departures[j + 1]);
        sum += this_dept - departures[j] - m_instance.p(curr_job, j);
        prev_dept = this_dept;
    }

    sum += prev_dept + m_instance.p(curr_job, m_instance.num_machines() - 1);

    return sum;
}

std::vector<size_t> SaDIWO::sort_inc_proc_time() {
    std::vector<size_t> sequence(m_instance.num_jobs());
    std::iota(sequence.begin(), sequence.end(), 0);

    std::vector<size_t> processing_times;
    processing_times.reserve(m_instance.num_jobs());
    for (size_t i = 0; i < m_instance.num_jobs(); ++i) {
        processing_times.push_back(m_instance.processing_times_sum()[i]);
    }

    std::sort(sequence.begin(), sequence.end(),
              [&processing_times](size_t a, size_t b) { return processing_times[a] < processing_times[b]; });

    return sequence;
}

Population SaDIWO::population_init() {
    const auto init_seq = sort_inc_proc_time();

    Solution best;

    // PF-NEH
    const int x = 5;
    for (size_t k = 0; k < x; ++k) {
        PF_NEH pf_neh(m_instance);
        // const auto seq = pf(init_seq, k);
        const size_t lambda = m_instance.num_jobs() >= 25 ? 25 : m_instance.num_jobs();
        const auto sol = pf_neh.solve(lambda);

        if (sol.cost < best.cost) {
            best = sol;
        }
    }

    // Calling it to set the departure times
    core::recalculate_solution(m_instance, best);

    Population pop;
    pop.add_solution(best);

    Solution tmp;
    tmp.sequence = best.sequence;

    // N_0 is P_max
    while (pop.solutions.size() < m_params.p_max) {
        std::shuffle(tmp.sequence.begin(), tmp.sequence.end(), m_rng);

        if (!pop.has_solution(tmp)) {
            core::recalculate_solution(m_instance, tmp);
            pop.add_solution(tmp);
        }
    }

    return pop;
}

size_t SaDIWO::get_solution_d(const Population &pop, const size_t solution_cost) {
    const auto t_worst = static_cast<double>(pop.solutions[pop.worst_solution_idx].cost);
    const auto t_best = static_cast<double>(pop.solutions[pop.best_solution_idx].cost);

    const double delta_ti = (t_worst - static_cast<double>(solution_cost)) / ((t_worst - t_best) + EPSILON);
    const double deviation =
        m_params.sigma_min + ((1 - std::exp(-delta_ti)) * (m_params.sigma_max - m_params.sigma_min));

    // floor is indicated in the accepted manuscript but in the published paper there is only the absolute value
    // since d needs to be an integer, floor is used
    size_t d = std::floor(std::abs(std::normal_distribution{0.0, std::pow(deviation, 2)}(m_rng)));

    // limiting it to the number of jobs for convenience
    return std::max(m_instance.num_jobs(), d);
}

void SaDIWO::spatial_dispersal(const Population &pop, Population &new_pop) {
    new_pop.solutions.clear();
    NEH helper(m_instance);
    for (size_t i = 0; i < pop.solutions.size(); ++i) {
        const auto &sol = pop.solutions[i];
        for (size_t j = 0; j < static_cast<size_t>(pop.seeds[i]); ++j) {
            const size_t d = get_solution_d(pop, sol.cost);

            Solution sol_copy;
            sol_copy.sequence = sol.sequence;
            std::vector<size_t> reverse_rel_pi(m_instance.num_jobs(), 0);
            for (size_t k = 0; k < m_instance.num_jobs(); ++k) {
                reverse_rel_pi[sol.sequence[k]] = k;
            }

            for (size_t k = 0; k < d; ++k) {
                const size_t max = m_instance.num_jobs() - k;
                const auto idx = static_cast<size_t>(std::floor(m_rng() % max));
                std::swap(sol_copy.sequence[idx], sol_copy.sequence[max - 1]);
            }

            // restoring non-removed jobs order to that of the original solution
            std::sort(
                sol_copy.sequence.begin(), sol_copy.sequence.end() - static_cast<long>(d),
                [&reverse_rel_pi](const size_t a, const size_t b) { return reverse_rel_pi[a] - reverse_rel_pi[b]; });

            for (size_t k = 0; k < d; ++k) {
                const size_t curr_job = sol_copy.sequence.back();
                sol_copy.sequence.pop_back();

                // TODO: check if it works
                // taillard_best_insert(sol_copy, m_instance.num_jobs() - d + k, curr_job);
                helper.taillard_best_insertion(sol_copy.sequence, curr_job);

            }

            new_pop.add_solution(sol_copy);
        }
    }
}

void SaDIWO::ls1(Solution &sol) {}

void SaDIWO::ls2(Solution &sol) {}

void SaDIWO::ls3(Solution &sol) {}

void SaDIWO::local_search(Population &pop) {
    for (auto &sol : pop.solutions) {
        bool improved = true;
        size_t previous_cost = sol.cost;
        while (improved) {
            // yes, in this order
            ls2(sol);
            ls3(sol);
            ls1(sol);

            if (previous_cost == sol.cost) {
                improved = false;
            } else {
                previous_cost = sol.cost;
            }
        }
    }
}

Solution SaDIWO::solve() {
    const long long max_time = 30 * static_cast<long long>(m_instance.num_jobs() * m_instance.num_machines());
    Clock clock;
    clock.start();
    auto pop = population_init();

    Population new_pop;
    while (clock.get_elapsed<std::chrono::milliseconds>() < max_time) {
        pop.calculate_seeds(m_params);
        spatial_dispersal(pop, new_pop);
        local_search(new_pop);

        // TODO: Competitive exclusion
    }

    clock.stop();
    return pop.solutions[pop.best_solution_idx];
}
