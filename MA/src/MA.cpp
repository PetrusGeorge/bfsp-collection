#include "MA.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>

namespace {
size_t uptime() {
    static const auto global_start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - global_start_time);
    return duration.count();
}
} // namespace

MA::MA(Instance instance, Parameters params) : m_instance(std::move(instance)), m_params(std::move(params)) {
    if (auto tl = m_params.time_limit()) {
        m_time_limit = *tl;
    } else {
        this->m_time_limit = m_params.ro() * m_instance.num_jobs() * m_instance.num_machines() / 1000;
    }
}

void MA::initialize_population() {

    const size_t n = m_instance.num_jobs();
    const size_t lambda = n > LAMBDA_MAX ? LAMBDA_MAX : n; // setting PF-NEH parameter

    m_pop = std::vector<Solution>(m_params.ps());

    PF_NEH pf_neh(m_instance);

    m_pop[0] = pf_neh.solve(lambda); // taking the first solution using PF-NEH
    core::recalculate_solution(m_instance, m_pop[0]);

    for (size_t i = 1; i < m_params.ps(); i++) {

        m_pop[i].sequence = generate_random_sequence();
        core::recalculate_solution(m_instance, m_pop[i]);
    }
}

std::vector<size_t> MA::generate_random_sequence() {

    std::vector<size_t> v(m_instance.num_jobs());
    std::iota(v.begin(), v.end(), 0);
    std::shuffle(v.begin(), v.end(), RNG::instance().gen());

    return v;
}

size_t MA::selection() {

    const size_t i = RNG::instance().generate((size_t)0, m_pop.size() - 1);
    size_t j = i;

    while (i == j) {
        j = RNG::instance().generate((size_t)0, m_pop.size() - 1);
    }

    if (m_pop[i].cost > m_pop[j].cost) {
        return j;
    }
    return i;
}

Solution MA::path_relink_swap(const Solution &beta, const Solution &pi) {

    Solution best;
    Solution current = beta;
    const size_t n = m_instance.num_jobs();

    /*
    Or the difference is 0 (and the solution are equal), or is greater than or equal to 2.
    If is less than or equal to 2, maybe one swap can make the solutions equal, hence it's
    applied a mutation.
    */
    size_t difference = 0;
    for (size_t k = 0; k < n; k++) {
        if (beta.sequence[k] != pi.sequence[k]) {

            difference++;
            if (difference > 2) {
                break;
            }
        }
    }

    if (difference <= 2) {
        mutation(current);
        core::recalculate_solution(m_instance, current);
        return current;
    }

    // cnt = number of jobs that are already in the correct position
    /*
    This part iterates over solution pi, finding where job i from solution
    beta is in solution pi. If i = j (j is the index of the job searched for
    in solution pi), then the jobs are in the same position (hence the correct
    position) and i = i+1 and we search for the next job. Otherwise, we swap
    the job at position i with the job at position j in solution beta, and
    move on to the next iteration.
    */
    size_t i = 0;
    for (size_t cnt = 0; cnt < n; cnt++) {

        if (current.sequence[i] == pi.sequence[i]) {
            i++;
            continue;
        }

        const size_t job = current.sequence[i];
        for (size_t j = i + 1; j < n; j++) {

            if (job != pi.sequence[j]) {
                continue;
            }

            std::swap(current.sequence[i], current.sequence[j]);
            core::partial_recalculate_solution(m_instance, current, i);

            if (current.cost < best.cost) {
                best = current; // new best interdiary solution
            }

            break;
        }
    }

    return best;
}

void MA::mutation(Solution &individual) {

    const size_t insertion_position = RNG::instance().generate((size_t)0, individual.sequence.size() - 1);
    size_t job_position = insertion_position;

    while (insertion_position == job_position) {
        job_position = RNG::instance().generate((size_t)0, individual.sequence.size() - 1);
    }

    individual.sequence.insert(individual.sequence.begin() + insertion_position, individual.sequence[job_position]);

    if (insertion_position > job_position) {
        individual.sequence.erase(individual.sequence.begin() + job_position);
    } else {
        individual.sequence.erase(individual.sequence.begin() + job_position + 1);
    }
}

bool MA::equal_solution(Solution &s1, Solution &s2) {

    if (s1.cost != s2.cost) {
        return false;
    }

    for (size_t i = 0; i < s1.sequence.size(); i++) {

        if (s1.sequence[i] != s2.sequence[i]) {
            return false;
        }
    }

    return true;
}

void MA::population_updating(std::vector<Solution> &offspring_population) {

    const size_t c = std::numeric_limits<size_t>::max();

    for (auto &i : offspring_population) {

        bool already_exist = false;
        size_t replaced_individual = c;
        for (size_t j = 0; j < m_pop.size(); j++) {

            if (equal_solution(i, m_pop[j])) {

                already_exist = true;
                break;
            }

            if (i.cost > m_pop[j].cost || replaced_individual != c) {
                continue;
            }

            replaced_individual = j;
        }

        if (!already_exist && replaced_individual != c) {
            m_pop.insert(m_pop.begin() + replaced_individual, i);
            m_pop.pop_back();
        }
    }
}

void MA::restart_population() {

    // half the jobs will mutate twice, and the other half will be generated randomly

    auto sort_criteria = [](Solution &p1, Solution &p2) { return p1.cost < p2.cost; };

    std::sort(m_pop.begin(), m_pop.end(), sort_criteria);

    for (size_t i = 0; i < m_pop.size(); i++) {

        if (i < m_pop.size() / 2) {
            mutation(m_pop[i]);
            mutation(m_pop[i]);
            core::recalculate_solution(m_instance, m_pop[i]);
        } else {
            m_pop[i].sequence = generate_random_sequence();
            core::recalculate_solution(m_instance, m_pop[i]);
        }
    }
}

Solution MA::solve() { // NOLINT

    Solution best_solution;
    std::vector<size_t> ref;
    const size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    std::vector<size_t> ro;
    if (m_params.benchmark()) {
        ro = {90, 60, 30};
    }

    initialize_population();

    auto sort_criteria = [](Solution &p1, Solution &p2) { return p1.cost < p2.cost; };

    std::sort(m_pop.begin(), m_pop.end(), sort_criteria);

    best_solution = m_pop[0];

    ref = best_solution.sequence;
    if (rls_grabowski(best_solution, ref, m_instance)) {
        core::recalculate_solution(m_instance, best_solution);
    }

    size_t count = 0;
    while (true) {

        std::vector<Solution> offspring_population;

        while (offspring_population.size() < m_params.ps()) {

            const size_t parent_1 = selection();
            size_t parent_2 = parent_1;
            while (parent_2 == parent_1) {
                parent_2 = selection();
            }

            Solution offspring1;
            Solution offspring2;
            if (RNG::instance().generate_real_number(0.0, 1.0) < m_params.pc()) {
                offspring1 = path_relink_swap(m_pop[parent_1], m_pop[parent_2]);
                offspring2 = path_relink_swap(m_pop[parent_2], m_pop[parent_1]);
            } else {
                offspring1 = m_pop[parent_1];
                offspring2 = m_pop[parent_2];
            }

            if (RNG::instance().generate_real_number(0.0, 1.0) < m_params.pm()) {
                mutation(offspring1);
                core::recalculate_solution(m_instance, offspring1);
            }
            if (RNG::instance().generate_real_number(0.0, 1.0) < m_params.pm()) {
                mutation(offspring2);
                core::recalculate_solution(m_instance, offspring2);
            }

            if (!equal_solution(offspring1, m_pop[parent_1]) && !equal_solution(offspring1, m_pop[parent_2])) {
                ref = offspring1.sequence;

                if (rls_grabowski(offspring1, ref, m_instance)) {
                    core::recalculate_solution(m_instance, offspring1);
                }

                offspring_population.push_back(offspring1);
            }
            if (!equal_solution(offspring2, m_pop[parent_1]) && !equal_solution(offspring2, m_pop[parent_2])) {
                ref = offspring2.sequence;

                if (rls_grabowski(offspring2, ref, m_instance)) {
                    core::recalculate_solution(m_instance, offspring2);
                }

                offspring_population.push_back(offspring2);
            }
        }

        std::sort(offspring_population.begin(), offspring_population.end(), sort_criteria);

        population_updating(offspring_population);

        count++;

        if (!ro.empty() && uptime() >= (ro.back() * mxn) / 1000) {

            std::cout << best_solution.cost << '\n';
            ro.pop_back();
        }

        if (uptime() > m_time_limit) {
            break;
        }

        if (m_pop[0].cost < best_solution.cost) {
            best_solution = m_pop[0];
            count = 0;
        }

        if (count >= m_params.gamma()) {
            count = 0;
            restart_population();
        }
    }

    m_pop.clear();
    return best_solution;
}