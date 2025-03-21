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
#include <numeric>

P_EDA::P_EDA(Instance &instance) : m_instance(instance) { m_pc.reserve(m_ps); }

Solution P_EDA::solve() { generate_random_individuals(); }

size_t P_EDA::calculate_similarity(const std::vector<size_t> &random_sequence, const Solution &individual) {
    assert(random_sequence.size() == m_instance.num_jobs());
    assert(individual.sequence.size() == m_instance.num_jobs());

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
            if (!similarity) {
                break;
            }
        }

        if (!similarity) {
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
    const size_t n = m_instance.num_jobs();

    PF pf(m_instance);
    NEH neh(m_instance);

    size_t lambda_pf_neh = n > 25 ? 25 : n;

    std::vector<size_t> sorted_jobs = core::stpt_sort(m_instance);

    size_t l = 0;

    while (m_pc.size() < (size_t)(m_ps * 0.1) && l < n) {

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
    }

    generate_random_individuals();
}
