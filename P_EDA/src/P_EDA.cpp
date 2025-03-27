#include "P_EDA.h"
#include "Core.h"
#include "Instance.h"
#include "Log.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"
#include "constructions/NEH.h"
#include "constructions/PF.h"
#include "local-search/RLS.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <numeric>

#include <chrono>
#include <future>
#include <atomic>
#include <thread>

namespace{
std::atomic<bool> time_expired(false); // Flag to track time

void timer_thread(size_t duration_seconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_seconds));
    time_expired = true; // Set flag when time is up
}
}

P_EDA::P_EDA(Instance &instance, Parameters &params, size_t ps, double lambda)
    : m_instance(instance), m_params(params), m_lambda(lambda), m_ps(ps) {
    m_pc.reserve(m_ps);
}

Solution P_EDA::solve() {
    size_t duration = m_instance.num_machines() * m_instance.num_jobs() * m_params.ro();

    std::cout << "Total running time: " << duration << " ms\n";

    auto timer_future = std::async(std::launch::async, timer_thread, duration);

    size_t gen = 0;
    generate_initial_population();
    modified_linear_rank_selection();

    // the p[i][j] represents how many times job j appeared before or in position i give the current population
    auto p = get_p();
    // the t[i][j][k] represents how many times job k appeared immideatly after job j in position i
    auto t = get_t();

    while (!time_expired) {

        Solution alpha;
        alpha.sequence = probabilistic_model(p, t);

        auto min_it = std::min_element(m_pc.begin(), m_pc.end(), [](const Solution &a, const Solution &b) {
            return a.cost < b.cost; // Compare costs
        });

        Solution best = path_relink_swap(alpha, *min_it);

        auto ref = fisher_yates_shuffle();

        rls(best, ref, m_instance);

        const auto [found_in_population, max_cost_pos] = in_population_and_max_makespan(best.sequence);
        const auto &max_cost_ind = m_pc[max_cost_pos];

        if (!found_in_population && best.cost < max_cost_ind.cost) {
            m_pc.erase(m_pc.begin() + max_cost_pos);
            m_pc.push_back(best);

            p = get_p();
            t = get_t();
        }

        gen = (gen + 1) % m_ps;
        if (gen == 0) {
            const auto &diversity = get_diversity();

            VERBOSE(m_params.verbose()) << "\ngen = 0...\n";

            if (diversity < m_lambda) {

                if (m_params.verbose()) {
                    std::cout << "diversity: " << diversity << ", lambda: " << m_lambda << "\n";
                    std::cout << "\nregenerating population...\n";
                }
                population_regen();
            }
        }
        if (m_params.verbose()) {
            std::cout << "best sequence: ";
            for (auto &j : best.sequence) {
                std::cout << j << " ";
            }
            std::cout << "best: " << best.cost << "\n";
            getchar();
        }
    }

    auto min_it = std::min_element(m_pc.begin(), m_pc.end(), [](const Solution &a, const Solution &b) {
        return a.cost < b.cost; // Compare costs
    });
    return *min_it;
}

size_t P_EDA::calculate_similarity(const std::vector<size_t> &random_sequence, const Solution &individual) {

    size_t similarity = 0;

    for (size_t i = 0; i < m_instance.num_jobs(); i++) {
        similarity += individual.sequence[i] == random_sequence[i] ? 0 : 1;
    }

    return similarity;
}

void P_EDA::generate_random_individuals() {

    std::vector<size_t> random_sequence(m_instance.num_jobs());
    std::iota(random_sequence.begin(), random_sequence.end(), 0);

    while (m_pc.size() < m_ps) {

        std::shuffle(random_sequence.begin(), random_sequence.end(), RNG::instance().gen());

        size_t similarity = 0;

        for (auto &ind : m_pc) {
            similarity = calculate_similarity(random_sequence, ind);
            if (similarity == 0) {
                break;
            }
        }

        if (similarity == 0) {
            continue;
        }

        Solution s;

        s.sequence = random_sequence;
        core::recalculate_solution(m_instance, s);
        m_pc.push_back(s);
    }
}

void P_EDA::generate_initial_population() {
    VERBOSE(m_params.verbose()) << "generating initial population...\n\n";
    const size_t n = m_instance.num_jobs();

    PF pf(m_instance);
    NEH neh(m_instance);

    const size_t lambda_pf_neh = n > 25 ? 25 : n;

    std::vector<size_t> sorted_jobs = core::stpt_sort(m_instance);

    size_t l = 0;

    const size_t pf_neh_individuals = m_ps / 10;

    while (m_pc.size() < pf_neh_individuals && l < n) {

        Solution s;
        const size_t first_job = sorted_jobs[l];

        pf.pf_insertion_phase(s, first_job);

        const std::vector<size_t> candidate_jobs = {s.sequence.begin() + (long)(n - lambda_pf_neh + 1),
                                                    s.sequence.end()};
        s.sequence = {s.sequence.begin(),
                      s.sequence.begin() +
                          (long)(n - lambda_pf_neh + 1)}; // needs to be n - lambda + 1 because [first_pos, last_post)

        neh.second_step(candidate_jobs, s);

        m_pc.push_back(s);

        l++;
    }

    generate_random_individuals();
    assert(m_pc.size() == m_ps);
}

void P_EDA::modified_linear_rank_selection() {
    std::vector<double> probabilities(m_ps);
    std::vector<double> sum_probabilities(m_ps);

    size_t l = m_ps - 1;
    // sorting individuals by descending makespan
    auto sort_criteria = [](Solution &a, Solution &b) { return a.cost > b.cost; };

    std::sort(m_pc.begin(), m_pc.end(), sort_criteria);

    const double sum_of_ranks = m_ps / 2 * (1 + m_ps);

    for (size_t i = 0; i < m_ps; i++) {
        probabilities[i] = (double)i + 1 / sum_of_ranks;

        if (i == 0) {
            sum_probabilities[i] = probabilities[i];
        } else {
            sum_probabilities[i] = sum_probabilities[i] + probabilities[i];
        }
    }
    std::vector<Solution> new_pc;

    while (new_pc.size() < m_ps) {
        const double lambda1 = RNG::instance().generate_real_number(0, 1);
        const double lambda2 = RNG::instance().generate_real_number(0, 1);

        if (lambda1 < lambda2) {
            new_pc.push_back(m_pc[l]);
            l--;
        } else {
            const double sigma = RNG::instance().generate_real_number(0, 1);
            for (size_t i = 1; i < m_ps; i++) {

                if (sum_probabilities[i - 1] <= sigma && sigma < sum_probabilities[i]) {
                    new_pc.push_back(m_pc[i]);
                }
            }
        }
    }
    m_pc = new_pc;
}

std::vector<size_t> P_EDA::probabilistic_model(const SizeTMatrix &p, const std::vector<SizeTMatrix> &t) {
    const size_t n = m_instance.num_jobs();

    std::vector<size_t> candidate_jobs(m_instance.num_jobs());
    std::iota(candidate_jobs.begin(), candidate_jobs.end(), 0);
    std::vector<size_t> final_sequence;
    final_sequence.reserve(n);

    while (final_sequence.size() < n) {
        auto probabilities = get_probability_vector(final_sequence, candidate_jobs, p, t);

        std::vector<double> roulette_wheel(candidate_jobs.size());

        roulette_wheel[0] = probabilities[0];

        size_t job_to_insert = candidate_jobs[0];
        const double r = RNG::instance().generate_real_number(0, 1);

        for (size_t j = 1; j < roulette_wheel.size(); j++) {
            roulette_wheel[j] = roulette_wheel[j - 1] + probabilities[j];
            if (roulette_wheel[j - 1] <= r && r < roulette_wheel[j]) {
                job_to_insert = candidate_jobs[j];
            }
        }
        final_sequence.push_back(job_to_insert);
        candidate_jobs.erase(std::remove(candidate_jobs.begin(), candidate_jobs.end(), job_to_insert));
    }
    return final_sequence;
}

std::vector<std::vector<size_t>> P_EDA::get_p() {
    const size_t n = m_instance.num_jobs();

    std::vector<std::vector<size_t>> p(n, std::vector<size_t>(n, 0));
    for (const auto &s : m_pc) {
        const auto &sequence = s.sequence;

        for (size_t i = 0; i < n; i++) {

            p[i][sequence[i]]++;
        }
    }

    for (size_t j = 0; j < n; j++) {
        for (size_t i = 1; i < n; i++) {
            p[i][j] += p[i - 1][j];
        }
    }
    return p;
}

std::vector<SizeTMatrix> P_EDA::get_t() {
    const size_t n = m_instance.num_jobs();
    std::vector<SizeTMatrix> t(n);

    for (auto &lambda_jk : t) {
        lambda_jk = std::vector<std::vector<size_t>>(n, std::vector<size_t>(n, 0));
    }

    for (auto &s : m_pc) {
        const auto &sequence = s.sequence;
        for (size_t i = 1; i < n; i++) {
            t[i][sequence[i - 1]][sequence[i]]++;
        }
    }

    return t;
}

std::vector<double> P_EDA::get_probability_vector(const std::vector<size_t> &sequence,
                                                  const std::vector<size_t> &candidate_jobs, const SizeTMatrix &p,
                                                  const std::vector<SizeTMatrix> &t) {
    const size_t n = m_instance.num_jobs();
    const size_t pos = sequence.size();

    std::vector<double> probabilities(candidate_jobs.size());
    if (pos == 0) {

        double sum_p = 0;
        for (const auto &j : candidate_jobs) {
            sum_p += (double)p[0][j];
        }

        for (size_t j = 0; j < n; j++) {
            const size_t job = candidate_jobs[j];
            probabilities[j] = (double)p[0][job] / sum_p;
        }
    } else {

        const size_t last_job = sequence.back();

        double sum_p = 0;
        double sum_t = 0;
        for (const auto &j : candidate_jobs) {
            sum_p += (double)p[0][j];
            sum_t += (double)t[pos][last_job][j];
        }

        for (size_t j = 0; j < candidate_jobs.size(); j++) {
            const size_t job = candidate_jobs[j];
            const double n_i_j_k =
                sum_t == 0 ? 1 / (double)candidate_jobs.size() : (double)t[pos][last_job][job] / sum_t;
            probabilities[j] = (double)p[0][job] / sum_p + n_i_j_k;
            ;
        }
    }

    return probabilities;
}

Solution P_EDA::path_relink_swap(const Solution &alpha, const Solution &beta) {

    Solution best;
    Solution current = alpha;
    const size_t n = m_instance.num_jobs();

    size_t difference = 0;
    for (size_t k = 0; k < n; k++) {
        if (alpha.sequence[k] != beta.sequence[k]) {
            difference++;
        }
    }

    if (difference <= 2) {
        mutation(current);
    }

    size_t i = 0;
    for (size_t cnt = 0; cnt < n; cnt++) {

        const size_t job = current.sequence[i];
        for (size_t j = 0; j < n; j++) {

            if (job != beta.sequence[j]) {
                continue;
            }

            if (i == j) {
                i++;
            } else {
                std::swap(current.sequence[i], current.sequence[j]);
                core::recalculate_solution(m_instance, current);

                if (current.cost < best.cost) {
                    best = current;
                }
            }

            break;
        }
    }

    return best;
}

void P_EDA::mutation(Solution &individual) {

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

    core::recalculate_solution(m_instance, individual);
}

std::vector<size_t> P_EDA::fisher_yates_shuffle() {

    std::vector<size_t> sequence(m_instance.num_jobs());
    std::iota(sequence.begin(), sequence.end(), 0);

    // Start from the last element and swap with random earlier element
    for (size_t i = sequence.size() - 1; i > 0; i--) {
        // Generate random index from 0 to i
        std::uniform_int_distribution<int> dist(0, i);
        int j = dist(RNG::instance().gen());

        // Swap elements at i and j
        std::swap(sequence[i], sequence[j]);
    }
    return sequence;
}

std::pair<bool, size_t> P_EDA::in_population_and_max_makespan(std::vector<size_t> &sequence) {

    size_t max_cost = 0;
    size_t max_cost_pos = 0;

    for (size_t i = 0; i < m_ps; i++) {
        const auto &ind = m_pc[i];
        size_t current_cost = ind.cost;
        if (current_cost > max_cost) {
            max_cost = current_cost;
            max_cost_pos = i;
        }

        bool found = true;
        const auto &seq = ind.sequence;

        for (size_t j = 0; j < m_instance.num_jobs(); j++) {
            if (seq[j] != sequence[j]) {
                found = false;
            }
        }
        if (found) {
            return {true, max_cost_pos};
        }
    }

    return {false, max_cost_pos};
}

double P_EDA::get_diversity() {
    const auto n = m_instance.num_jobs();
    double diversity = 0;

    SizeTMatrix c(n, std::vector<size_t>(n, 0));

    for (const auto &s : m_pc) {
        const auto &sequence = s.sequence;

        for (size_t i = 0; i < n; i++) {

            c[i][sequence[i]]++;
        }
    }

    for (size_t k = 0; k < n; k++) {
        for (size_t alpha = 0; alpha < n; alpha++) {

            diversity += ((double)c[k][alpha] / m_ps) * (1 - (double)c[k][alpha] / m_ps);
        }
    }

    diversity /= (double)n - 1;

    return diversity;
}
void P_EDA::population_regen() {
    auto sort_criteria = [](Solution &a, Solution &b) { return a.cost < b.cost; };

    std::sort(m_pc.begin(), m_pc.end(), sort_criteria);

    // removing last 40% jobs of the population
    m_pc = {m_pc.begin(), m_pc.end() - 2 * (m_ps / 5)};

    // keep 1/3 of the new population (20% of previous population)
    for (size_t j = m_ps / 3 + 1; j < m_pc.size(); j++) {
        auto &current_individual = m_pc[j];
        auto &current_seq = current_individual.sequence;

        // random reinsert
        auto pos = RNG::instance().generate((size_t)0, m_pc.size());
        auto job = current_seq[pos];
        current_seq.erase(current_seq.begin() + pos);
        pos = RNG::instance().generate((size_t)0, m_pc.size());
        current_seq.insert(current_seq.begin() + pos, job);
    }
    generate_random_individuals();
}

void P_EDA::print_pc() const {
    std::cout << "Population: \n";
    for (size_t i = 0; i < m_pc.size(); i++) {
        std::cout << "individual " << i << ": \n" << m_pc[i] << "\n";
    }
}

void P_EDA::print_count(std::vector<std::vector<size_t>> &count) {
    std::cout << "\ncount matrix: \n";
    for (auto &row : count) {
        for (auto &c : row) {
            std::cout << c << " ";
        }
        std::cout << "\n";
    }
}
