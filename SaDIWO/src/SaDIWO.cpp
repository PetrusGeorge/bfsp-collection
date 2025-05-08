#include <algorithm>
#include <limits>
#include <numeric>
#include <random>

#include "Clock.h"
#include "Instance.h"
#include "SaDIWO.h"
#include "Solution.h"

static const double EPSILON = 1e-15; // Assumes that the double type is IEEE 754 compliant

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

    return true;
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

void SaDIWO::update_departure_times(std::vector<long> &departures, const size_t curr_job) const {
    long prev_departure = departures[0];
    for (size_t j = 0; j < m_instance.num_machines() - 1; ++j) {
        const long tmp = departures[j];
        departures[j] = std::max(prev_departure + m_instance.p(curr_job, j), departures[j + 1]);
        prev_departure = tmp;
    }
    const size_t last_j = m_instance.num_machines() - 1;
    departures[last_j] = departures[last_j - 1] + m_instance.p(curr_job, last_j);
}

long SaDIWO::get_idle_block_sum(const std::vector<long> &departures, const size_t curr_job) const {
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

std::vector<size_t> SaDIWO::pf(const std::vector<size_t> &init_seq, const size_t pos) const {
    std::vector<size_t> seq(m_instance.num_jobs(), 0);

    size_t unscheduled_size = m_instance.num_jobs();
    std::vector<size_t> unscheduled(unscheduled_size);
    std::iota(unscheduled.begin(), unscheduled.end(), 0);

    seq[0] = init_seq[pos];
    std::swap(unscheduled[--unscheduled_size], unscheduled[pos]);

    std::vector<long> departures(m_instance.num_machines(), 0);

    for (size_t idx = 0; idx < m_instance.num_jobs() - 1; ++idx) {
        const size_t curr_job = seq[idx];
        update_departure_times(departures, curr_job);

        size_t selected = 0;
        long best_idle_block_sum = std::numeric_limits<long>::max();
        for (size_t j = 0; j < unscheduled_size; ++j) {
            const long curr_idle_block_sum = get_idle_block_sum(departures, unscheduled[j]);

            if (curr_idle_block_sum < best_idle_block_sum) {
                best_idle_block_sum = curr_idle_block_sum;
                selected = j;
            }
        }

        seq[idx + 1] = unscheduled[selected];
        std::swap(unscheduled[--unscheduled_size], unscheduled[selected]);
    }

    seq[m_instance.num_jobs() - 1] = unscheduled[0];

    return seq;
}

std::vector<std::vector<long>> SaDIWO::compute_tail(const std::vector<size_t> &pi, const size_t q) const {
    if (q == 0) {
        return {};
    }

    std::vector<std::vector<long>> tail(q, std::vector<long>(m_instance.num_machines(), 0));

    for (size_t i = q - 1; i >= 0; --i) {
        for (size_t j = 0; j < m_instance.num_machines() - 1; ++j) {
            tail[i][j] = std::max(tail[i][j + 1], tail[i + 1][j]) + m_instance.p(pi[i], j);
        }
        const size_t m = m_instance.num_machines() - 1;
        tail[i][m] = tail[i + 1][m] + m_instance.p(pi[i], m);
    }

    return tail;
}

std::vector<std::vector<long>> SaDIWO::compute_completion_times(const std::vector<size_t> &pi, const size_t q) const {
    if (q == 0) {
        return {};
    }

    std::vector<std::vector<long>> completion(q, std::vector<long>(m_instance.num_jobs(), 0));

    completion[0][0] = m_instance.p(pi[0], 0);
    for (size_t j = 1; j < m_instance.num_machines(); ++j) {
        completion[0][j] = completion[0][j - 1] + m_instance.p(pi[0], j);
    }

    for (size_t i = 1; i < q; ++i) {
        completion[i][0] = completion[i - 1][0] + m_instance.p(pi[i], 0);

        for (size_t j = 1; j < m_instance.num_machines(); ++j) {
            completion[i][j] = std::max(completion[i - 1][j], completion[i][j - 1]) + m_instance.p(pi[i], j);
        }
    }

    return completion;
}

void SaDIWO::taillard_best_insert(Solution &final_sol, const size_t q, const size_t curr_job) const {
    const auto tail = compute_tail(final_sol.sequence, q);
    const auto completion = compute_completion_times(final_sol.sequence, q);

    std::vector<std::vector<long>> rel_completion(q, std::vector<long>(m_instance.num_machines(), 0));

    const long t = m_instance.p(curr_job, 0);

    // First job and first machine
    rel_completion[0][0] = t;

    long best_partial_makespan = rel_completion[0][0];

    for (size_t j = 1; j < m_instance.num_machines(); ++j) {
        rel_completion[0][j] = rel_completion[0][j - 1] + t;

        best_partial_makespan = std::max(rel_completion[0][j] + tail[0][j], best_partial_makespan);
    }

    size_t best_index = 0;
    for (size_t i = 1; i < q + 1; ++i) {
        rel_completion[i][0] = completion[i - 1][0] + t;

        long partial_makespan = rel_completion[i][0];
        for (size_t j = 1; j < m_instance.num_machines(); ++j) {
            rel_completion[i][j] = std::max(rel_completion[i][j - 1], completion[i - 1][j]) + t;

            partial_makespan = std::max(rel_completion[i][j] + tail[i][j], partial_makespan);
        }

        if (partial_makespan < best_partial_makespan) {
            best_partial_makespan = partial_makespan;
            best_index = i;
        }
    }

    final_sol.sequence.insert(final_sol.sequence.begin() + static_cast<long>(best_index), curr_job);
    final_sol.cost = best_partial_makespan;
}

Solution SaDIWO::neh(const std::vector<size_t> &seq) const {
    const size_t lambda = m_instance.num_jobs() >= 25 ? 25 : m_instance.num_jobs();

    Solution final_sol;
    final_sol.cost = 0;
    final_sol.sequence.reserve(m_instance.num_jobs());
    final_sol.sequence.assign(seq.cbegin(), seq.cend() - static_cast<long>(lambda));

    for (size_t q = m_instance.num_jobs() - lambda; q < m_instance.num_jobs(); ++q) {
        taillard_best_insert(final_sol, q, seq[q]);
    }

    return final_sol;
}

std::vector<size_t> SaDIWO::sort_inc_proc_time() const {
    std::vector<size_t> sequence(m_instance.num_jobs());
    std::iota(sequence.begin(), sequence.end(), 0);

    std::vector<long> processing_times;
    processing_times.reserve(m_instance.num_jobs());
    for (size_t i = 0; i < m_instance.num_jobs(); ++i) {
        processing_times.push_back(m_instance.psum(i));
    }

    std::sort(sequence.begin(), sequence.end(),
              [&processing_times](size_t a, size_t b) { return processing_times[a] < processing_times[b]; });

    return sequence;
}

Population SaDIWO::population_init() const {
    const auto init_seq = sort_inc_proc_time();

    Solution best;

    // PF-NEH
    const int x = 5;
    for (size_t k = 0; k < x; ++k) {
        const auto seq = pf(init_seq, k);
        const auto sol = neh(seq);

        if (sol.cost < best.cost) {
            best = sol;
        }
    }

    // Calling it to set the departure times
    set_solution_cost(m_instance, best);

    Population pop;
    pop.add_solution(best);

    std::random_device rd;
    std::mt19937 g(rd());

    Solution tmp;
    tmp.sequence = best.sequence;

    // N_0 is P_max
    while (pop.solutions.size() < m_params.p_max) {
        std::shuffle(tmp.sequence.begin(), tmp.sequence.end(), g);

        if (!pop.has_solution(tmp)) {
            set_solution_cost(m_instance, tmp);
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
                taillard_best_insert(sol_copy, m_instance.num_jobs() - d + k, curr_job);
            }

            new_pop.add_solution(sol_copy);
        }
    }
}

void ls1(Solution &sol) const {}

void ls2(Solution &sol) const {}

void ls3(Solution &sol) const {}

void SaDIWO::local_search(Population &pop) const {
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
