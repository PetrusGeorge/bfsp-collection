#include "P_EDA.h"
#include "Core.h"
#include "Instance.h"
#include "RNG.h"
#include "Solution.h"
#include "constructions/NEH.h"
#include "constructions/PF.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <numeric>

P_EDA::P_EDA(Instance &instance) : m_instance(instance) { m_pc.reserve(m_ps); }

Solution P_EDA::solve() {
    generate_initial_population();
    modified_linear_rank_selection();

    print_pc();
    Solution alpha;
    alpha.sequence = probabilistic_model();
    auto min_it = std::min_element(m_pc.begin(), m_pc.end(), [](const Solution &a, const Solution &b) {
        return a.cost < b.cost; // Compare costs
    });
    Solution best = path_relink_swap(alpha, *min_it);
    std::cout << "best sequence: ";
    for (auto &j : best.sequence) {
        std::cout << j << " ";
    }
    std::cout << "best: " << best.cost << "\n";

    return best;
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
        std::vector<std::vector<size_t>> d_final = core::calculate_departure_times(m_instance, s.sequence);
        s.cost = d_final.back()[m_instance.num_machines() - 1];
        m_pc.push_back(s);
    }
}

void P_EDA::generate_initial_population() {
    std::cout << "generating population...\n";
    const size_t n = m_instance.num_jobs();

    PF pf(m_instance);
    NEH neh(m_instance);

    const size_t lambda_pf_neh = n > 25 ? 25 : n;

    std::vector<size_t> sorted_jobs = core::stpt_sort(m_instance);

    size_t l = 0;

    const size_t pf_neh_individuals = m_ps / 10;

    while (m_pc.size() < pf_neh_individuals && l < n) {
        std::cout << "pc size: " << m_pc.size() << "\n";

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

std::vector<size_t> P_EDA::probabilistic_model() {
    const size_t n = m_instance.num_jobs();

    // the p[i][j] represents how many times job j appeared before or in position i give the current population
    auto p = get_p();
    // the t[i][j][k] represents how many times job k appeared immideatly after job j in position i
    auto t = get_t();

    std::vector<size_t> candidate_jobs(m_instance.num_jobs());
    std::iota(candidate_jobs.begin(), candidate_jobs.end(), 0);
    std::vector<size_t> final_sequence;
    final_sequence.reserve(n);

    print_count(p);
    getchar();

    while (final_sequence.size() < n) {
        auto probabilities = get_probability_vector(final_sequence, candidate_jobs, p, t);

        std::vector<double> roulette_wheel(candidate_jobs.size());

        // VERBOSE(true) << "candidate_jobs: ";
        // for (const auto &p : candidate_jobs) {
        //     std::cout << p << " ";
        // }
        // std::cout << "\n";

        roulette_wheel[0] = probabilities[0];

        size_t job_to_insert = candidate_jobs[0];
        const double r = RNG::instance().generate_real_number(0, 1);
        // std::cout << "r: " << r << "\n";
        // std::cout << "roulette wheel: ";

        for (size_t j = 1; j < roulette_wheel.size(); j++) {
            roulette_wheel[j] = roulette_wheel[j - 1] + probabilities[j];
            // std::cout << roulette_wheel[j] << " ";
            if (roulette_wheel[j - 1] <= r && r < roulette_wheel[j]) {
                job_to_insert = candidate_jobs[j];
            }
        }
        // std::cout << "job chosen: " << job_to_insert << "\n\n";
        final_sequence.push_back(job_to_insert);
        candidate_jobs.erase(std::remove(candidate_jobs.begin(), candidate_jobs.end(), job_to_insert));

        // std::cout << "sequence: ";
        // for(auto& j : final_sequence){
        //     std::cout << j << " ";
        // }
        // std::cout << "\n";
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
