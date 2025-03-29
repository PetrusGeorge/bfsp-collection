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

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <utility>

namespace {
std::atomic<bool> time_expired(false); // Flag to track time

void timer_thread(size_t duration_seconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_seconds));
    time_expired = true; // Set flag when time is up
}
} // namespace

P_EDA::P_EDA(Instance &instance, Parameters &params, size_t ps, double lambda)
    : m_instance(instance), m_params(params), m_lambda(lambda), m_ps(ps) {
    m_pc.reserve(m_ps);
}

Solution P_EDA::solve() {
    const size_t duration = m_instance.num_machines() * m_instance.num_jobs() * m_params.ro();

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

        Solution alpha = probabilistic_model(p, t);

        auto min_it = std::min_element(m_pc.begin(), m_pc.end(), [](const Solution &a, const Solution &b) {
            return a.cost < b.cost; // Compare costs
        });

        Solution best = path_relink_swap(alpha, *min_it, 0);

        auto ref = fisher_yates_shuffle();

        rls(best, ref, m_instance);

        const auto [found_in_population, max_cost_pos] = in_population_and_max_makespan(best.sequence);
        const auto &max_cost = m_pc[max_cost_pos].cost;

        if (!found_in_population && best.cost < max_cost) {
            m_pc.erase(m_pc.begin() + max_cost_pos);
            m_pc.push_back(best);

            p = get_p();
            t = get_t();
        }

        gen = (gen + 1) % m_ps;
        if (gen == 0) {

            VERBOSE(m_params.verbose()) << "\ngen = 0...\n";

            const auto &diversity = get_diversity();
            if (diversity < m_lambda) {

                if (m_params.verbose()) {
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
        }
    }

    auto min_it = std::min_element(m_pc.begin(), m_pc.end(), [](const Solution &a, const Solution &b) {
        return a.cost < b.cost; // Compare costs
    });
    return *min_it;
}

void P_EDA::generate_random_individuals() {

    std::vector<size_t> random_sequence(m_instance.num_jobs());
    std::iota(random_sequence.begin(), random_sequence.end(), 0);

    while (m_pc.size() < m_ps) {

        std::shuffle(random_sequence.begin(), random_sequence.end(), RNG::instance().gen());

        size_t similarity = 0;

        for (auto &ind : m_pc) {

            for (size_t i = 0; i < m_instance.num_jobs(); i++) {
                if (ind.sequence[i] != random_sequence[i]) {
                    similarity++;
                }
            }

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

    const size_t lambda_pf_neh = std::min((size_t)25, n);

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
    std::vector<double> sum_probabilities(m_ps + 1);

    size_t l = m_ps - 1;
    // sorting individuals by descending makespan
    auto sort_criteria = [](Solution &a, Solution &b) { return a.cost > b.cost; };

    std::sort(m_pc.begin(), m_pc.end(), sort_criteria);

    const double sum_of_ranks = m_ps / 2 * (1 + m_ps);

    sum_probabilities[0] = 0;

    for (size_t i = 1; i <= m_ps; i++) {
        sum_probabilities[i] = sum_probabilities[i - 1] + (double)i / sum_of_ranks;
    }

    std::vector<Solution> new_pc;
    new_pc.reserve(m_ps);

    while (new_pc.size() < m_ps) {
        const double lambda1 = RNG::instance().generate_real_number(0, 1);
        const double lambda2 = RNG::instance().generate_real_number(0, 1);

        if (lambda1 < lambda2) {
            new_pc.push_back(m_pc[l]);
            l--;
        } else {
            const double sigma = RNG::instance().generate_real_number(0, 1);

            for (size_t i = 1; i <= m_ps; i++) {

                if (sum_probabilities[i - 1] <= sigma && sigma < sum_probabilities[i]) {
                    new_pc.push_back(m_pc[i - 1]);
                    break;
                }
            }
        }
    }
    m_pc = new_pc;
}

Solution P_EDA::probabilistic_model(const SizeTMatrix &p, const std::vector<SizeTMatrix> &t) {
    const size_t n = m_instance.num_jobs();

    std::vector<size_t> unasigned_jobs(m_instance.num_jobs());
    std::iota(unasigned_jobs.begin(), unasigned_jobs.end(), 0);
    std::vector<size_t> final_sequence;
    final_sequence.reserve(n);

    while (final_sequence.size() < n - 1) {
        auto probabilities = get_probability_vector(final_sequence, unasigned_jobs, p, t);

        double roulette_wheel = 0;

        size_t job_to_insert = unasigned_jobs[0];
        const double r = RNG::instance().generate_real_number(0, 1);

        for (size_t j = 0; j < unasigned_jobs.size(); j++) {

            if (roulette_wheel <= r && r < roulette_wheel + probabilities[j]) {
                job_to_insert = unasigned_jobs[j];
            }
            roulette_wheel += probabilities[j];
        }
        final_sequence.push_back(job_to_insert);
        unasigned_jobs.erase(std::remove(unasigned_jobs.begin(), unasigned_jobs.end(), job_to_insert));
    }
    final_sequence.push_back(unasigned_jobs.front());

    Solution s;
    s.sequence = final_sequence;
    core::recalculate_solution(m_instance, s);
    return s;
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
                                                  const std::vector<size_t> &unasigned_jobs, const SizeTMatrix &p,
                                                  const std::vector<SizeTMatrix> &t) {
    const size_t n = m_instance.num_jobs();
    std::vector<double> probabilities(unasigned_jobs.size());

    if (sequence.empty()) {

        double sum_p = 0;
        for (const auto &j : unasigned_jobs) {
            sum_p += (double)p[0][j];
        }

        for (size_t j = 0; j < n; j++) {
            const size_t job = unasigned_jobs[j];
            probabilities[j] = (double)p[0][job] / sum_p;
        }
    } else {

        const size_t last_job = sequence.back();
        const size_t pos = sequence.size(); // the position the new job will be inserted

        double sum_p = 0;
        double sum_t = 0;
        for (const auto &j : unasigned_jobs) {
            sum_p += (double)p[pos][j];
            sum_t += (double)t[pos][last_job][j];
        }

        for (size_t j = 0; j < unasigned_jobs.size(); j++) {
            const size_t job = unasigned_jobs[j];

            double n_i_j_k = -1;

            if (sum_t == 0) {
                n_i_j_k = 1 / (double)unasigned_jobs.size();
            } else {
                n_i_j_k = (double)t[pos][last_job][job] / sum_t;
            }

            probabilities[j] = (double)p[pos][job] / sum_p + n_i_j_k;
            probabilities[j] /= 2;
        }
    }

    return probabilities;
}

Solution P_EDA::path_relink_swap(const Solution &alpha, const Solution &beta, size_t similarity_threshold) {

    Solution best;
    Solution current = alpha;
    const size_t n = m_instance.num_jobs();

    bool similar = is_similar(current.sequence, beta.sequence, similarity_threshold);

    if (similar) {
        mutation(current);
    }

    best = current;

    for (size_t k = 0; k < n; k++) {
        const size_t current_job = beta.sequence[k];

        auto p = std::find(current.sequence.begin(), current.sequence.end(), current_job);
        std::swap(current.sequence[k], *p);

        similar = is_similar(current.sequence, beta.sequence, similarity_threshold);
        if (similar) {
            break;
        }
        if (current.cost < best.cost) {
            best = current;
        }
    }

    return best;
}

inline void P_EDA::mutation(Solution &individual) {

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
        const size_t j = RNG::instance().generate(size_t{0}, i);

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
        const size_t current_cost = ind.cost;
        if (current_cost > max_cost) {
            max_cost = current_cost;
            max_cost_pos = i;
        }

        bool found = true;
        const auto &seq = ind.sequence;

        for (size_t j = 0; j < m_instance.num_jobs(); j++) {
            if (seq[j] != sequence[j]) {
                found = false;
                break;
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

            diversity += ((double)c[k][alpha] / m_ps) * (1 - ((double)c[k][alpha] / m_ps));
        }
    }
    VERBOSE(m_params.verbose()) << "diversity without div: " << diversity << "\n";

    diversity /= (double)n - 1;

    return diversity;
}
void P_EDA::population_regen() {
    auto sort_criteria = [](Solution &a, Solution &b) { return a.cost < b.cost; };

    std::sort(m_pc.begin(), m_pc.end(), sort_criteria);

    // removing last 40% jobs of the population
    m_pc = {m_pc.begin(), m_pc.end() - 2 * (m_ps / 5)};

    // keep 20% of the population and do a random insertion in the 40% intermediates
    for (size_t j = (m_ps / 5) + 1; j < m_pc.size(); j++) {
        auto &current_individual = m_pc[j];
        auto &current_seq = current_individual.sequence;

        // random reinsert
        auto pos = RNG::instance().generate((size_t)0, current_seq.size() - 1);
        auto job = current_seq[pos];
        current_seq.erase(current_seq.begin() + pos);
        pos = RNG::instance().generate((size_t)0, current_seq.size());
        current_seq.insert(current_seq.begin() + pos, job);
        core::recalculate_solution(m_instance, current_individual);
    }
    generate_random_individuals();
}

inline bool P_EDA::is_similar(const std::vector<size_t> &a, const std::vector<size_t> &b, size_t similarity_threshold) {

    size_t diference = 0;
    for (size_t j = 0; j < a.size(); j++) {

        if (a[j] != b[j]) {
            diference++;
            if (diference > similarity_threshold) {
                return false;
            }
        }
    }
    return true;
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
