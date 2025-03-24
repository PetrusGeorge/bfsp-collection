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
    Solution s;
    generate_initial_population();
    calculate_probabilities();
    return s;
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

    size_t lambda_pf_neh = n > 25 ? 25 : n;

    std::vector<size_t> sorted_jobs = core::stpt_sort(m_instance);

    size_t l = 0;

    size_t pf_neh_individuals = m_ps / 10;

    while (m_pc.size() < pf_neh_individuals && l < n) {
        std::cout << "pc size: " << m_pc.size() << "\n";

        Solution s;
        size_t first_job = sorted_jobs[l];

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

    double sum_of_ranks = m_ps / 2 * (1 + m_ps);

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
        double lambda1 = RNG::instance().generate_real_number(0, 1);
        double lambda2 = RNG::instance().generate_real_number(0, 1);

        if (lambda1 < lambda2) {
            new_pc.push_back(m_pc[l]);
            l--;
        } else {
            double delta = RNG::instance().generate_real_number(0, 1);
            for (size_t i = 1; i < m_ps; i++) {

                if (sum_probabilities[i - 1] <= delta && delta < sum_probabilities[i]) {
                    new_pc.push_back(m_pc[i]);
                }
            }
        }
    }
    m_pc = new_pc;
}

void P_EDA::population_sampling() {}
std::vector<std::vector<double>> P_EDA::calculate_probabilities() {
    const size_t n = m_instance.num_jobs();

    // matrix that represents the probabilitie of the job j being at position i given the current population
    std::vector<std::vector<double>> prob_pos_i_job_j(n, std::vector<double>(n));
    auto count = count_in_population();
    print_pc();
    print_count(count);

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {

            for (auto &s : m_pc) {
            }
        }
    }
    return prob_pos_i_job_j;
}

std::vector<std::vector<size_t>> P_EDA::count_in_population() {
    const size_t n = m_instance.num_jobs();

    std::vector<std::vector<size_t>> count_pos_i_job_j(n, std::vector<size_t>(n, 0));
    for (auto &s : m_pc) {
        auto &sequence = s.sequence;

        for (size_t i = 0; i < n; i++) {

            count_pos_i_job_j[i][sequence[i]]++;
        }
    }
    return count_pos_i_job_j;
}

void P_EDA::print_pc() const {
    std::cout << "Population: \n";
    for (size_t i = 0; i < m_pc.size(); i++) {
        std::cout << "individual " << i << ": \n" << m_pc[i] << "\n";
    }
}

void P_EDA::print_count(std::vector<std::vector<size_t>> &count) const {
    std::cout << "\ncount matrix: \n";
    for (auto &row : count) {
        for (auto &c : row) {
            std::cout << c << " ";
        }
        std::cout << "\n";
    }
}
