#include <algorithm>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>

#include "Core.h"
#include "Instance.h"
#include "NEH.h"
#include "PF_NEH.h"
#include "Parameters.h"
#include "RLS.h"
#include "SaDIWO.h"
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
    const size_t cost = solution.cost;
    solutions.push_back(std::move(solution));

    if (solutions.size() == 1) {
        best_solution_idx = 0;
        worst_solution_idx = 0;
        return;
    }

    if (cost < solutions[best_solution_idx].cost) {
        best_solution_idx = solutions.size() - 1;
    } else if (cost > solutions[worst_solution_idx].cost) {
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

void Population::calculate_seeds(const Parameters &params) {
    seeds.resize(solutions.size());

    const size_t t_worst = solutions[worst_solution_idx].cost;
    const size_t t_best = solutions[best_solution_idx].cost;
    const double multiplier = static_cast<double>((params.s_max() - params.s_min()) + params.s_min());

    for (size_t i = 0; i < solutions.size(); ++i) {
        const double div =
            static_cast<double>(t_worst - solutions[i].cost) / (static_cast<double>(t_worst - t_best) + EPSILON);
        seeds[i] = std::floor(div * multiplier);
    }
}

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
    while (pop.solutions.size() < m_params.p_max()) {
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
        static_cast<double>(m_params.sigma_min()) +
        ((1 - std::exp(-delta_ti)) * static_cast<double>(m_params.sigma_max() - m_params.sigma_min()));

    // Floor is indicated in the accepted manuscript but in the published paper there is only the absolute value.
    // Since d needs to be an integer, std::floor is used
    const size_t d = std::floor(std::abs(std::normal_distribution{0.0, std::pow(deviation, 2)}(m_rng)));

    // limiting it to the number of jobs minus 1 for convenience
    return std::min(m_instance.num_jobs() - 1, d);
}

Population SaDIWO::spatial_dispersal(const Population &pop) {
    Population new_pop;
    new_pop.solutions.clear();
    NEH helper(m_instance);
    for (size_t i = 0; i < pop.solutions.size(); ++i) {
        const auto &sol = pop.solutions[i];
        for (size_t j = 0; j < static_cast<size_t>(pop.seeds[i]); ++j) {
            const size_t d = get_solution_d(pop, sol.cost);

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
            helper.second_step(std::move(pi_r), sol_copy);
            new_pop.add_solution(std::move(sol_copy));
        }
    }

    return new_pop;
}

void SaDIWO::local_search(Population &pop) {
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

Population SaDIWO::competitive_exclusion(Population pop, Population new_pop) const {
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

Solution SaDIWO::solve() {
    const size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    size_t time_limit = m_params.ro() * m_instance.num_jobs() * m_instance.num_machines();
    std::vector<size_t> ro;
    if (m_params.benchmark()) {
        time_limit = 100 * mxn; // RO == 100
        ro = {90, 60, 30};
    }

    Population pop = population_init();
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

        pop.calculate_seeds(m_params);
        Population new_pop = spatial_dispersal(pop);
        local_search(new_pop);

        pop = competitive_exclusion(std::move(pop), std::move(new_pop));
    }

    return best;
}
