#include "algorithms/LPT.h"
#include "Core.h"
#include <algorithm>
#include <numeric>

LPT::LPT(Instance &instance, Parameters &params, bool jobs_reversed)
    : m_instance(instance), m_params(params), m_reversed(jobs_reversed) {}

Solution LPT::solve() {
    Solution s;
    s.sequence.reserve(m_instance.num_jobs());

    m_phi = initial_job_sequence();

    s.sequence = {m_phi[0]};
    m_phi.erase(m_phi.begin());

    while (!m_phi.empty()) {
        set_taillard_matrices(s.sequence, m_phi.front());

        auto [best_index, makespan] = get_best_insertion();
        s.sequence.insert(s.sequence.begin() + (long)best_index, m_phi.front());

        s.cost = makespan;
        m_phi.erase(m_phi.begin());
    }

    return s;
}

std::vector<size_t> LPT::initial_job_sequence() {
    std::vector<size_t> sequence(m_instance.num_jobs());
    std::iota(sequence.begin(), sequence.end(), 0);

    auto p = core::get_reversible_matrix(m_instance, m_reversed);

    std::ranges::sort(sequence, [p, this](size_t a, size_t b) {
        size_t sum_a = 0;

        for (size_t j = 0; j < m_instance.num_machines(); j++) {
            sum_a += p(a, j);
        }
        size_t sum_b = 0;

        for (size_t j = 0; j < m_instance.num_machines(); j++) {
            sum_b += p(b, j);
        }
        return sum_a > sum_b;
    });

    return sequence;
}

void LPT::set_taillard_matrices(const std::vector<size_t> &sequence, size_t k) {
    m_e = core::calculate_departure_times(m_instance, sequence, m_reversed);

    m_q = calculate_tail(sequence);
    m_q.emplace_back(m_instance.num_machines(),
                     0); // Make it easier to implement find_best_insertion without an out of bound access

    auto p = core::get_reversible_matrix(m_instance, m_reversed);

    // f needs to store all possibilities of insertion so it has sequence.size + 1
    m_f = std::vector(sequence.size() + 1, std::vector<size_t>(m_instance.num_machines()));
    m_f[0][0] = p(k, 0);
    for (size_t j = 1; j < m_instance.num_machines(); j++) {
        m_f[0][j] = m_f[0][j - 1] + p(k, j);
    }
    for (size_t i = 1; i <= sequence.size(); i++) {
        m_f[i][0] = std::max(m_e[i - 1][0] + p(k, 0), m_e[i - 1][1]);
        for (size_t j = 1; j < m_instance.num_machines() - 1; j++) {
            m_f[i][j] = std::max(m_f[i][j - 1] + p(k, j), m_e[i - 1][j + 1]);
        }
        m_f[i].back() = m_f[i][m_instance.num_machines() - 2] + p(k, m_instance.num_machines() - 1);
    }
}

std::vector<std::vector<size_t>> LPT::calculate_tail(const std::vector<size_t> &sequence) {

    auto tail = std::vector(sequence.size(), std::vector<size_t>(m_instance.num_machines()));

    const std::function<long(size_t, size_t)> p = core::get_reversible_matrix(m_instance, m_reversed);

    // Calculate first job
    tail.back().back() = p(sequence.back(), m_instance.num_machines() - 1);
    for (long j = (long)m_instance.num_machines() - 2; j >= 0; j--) {
        tail.back()[j] = tail.back()[j + 1] + p(sequence.back(), j);
    }

    for (long i = ((long)sequence.size()) - 2; i >= 0; i--) {
        const size_t node = sequence[i];
        tail[i].back() = std::max(tail[i + 1].back() + p(node, m_instance.num_machines() - 1),
                                  tail[i + 1][m_instance.num_machines() - 2]);
        for (long j = (long)m_instance.num_machines() - 2; j >= 1; j--) {

            const size_t current_finish_time = tail[i][j + 1] + p(node, j);

            tail[i][j] = std::max(current_finish_time, tail[i + 1][j - 1]);
        }
        tail[i][0] = tail[i][1] + p(node, 0);
    }

    return tail;
}

std::pair<size_t, size_t> LPT::get_best_insertion() {
    size_t best_index = 0;
    size_t best_value = std::numeric_limits<size_t>::max();

    for (size_t i = 0; i < m_f.size(); i++) {
        size_t max_value = 0;
        for (size_t j = 0; j < m_f[i].size(); j++) {
            const size_t sum = m_f[i][j] + m_q[i][j];
            max_value = std::max(sum, max_value);
        }

        if (max_value < best_value) {
            best_value = max_value;
            best_index = i;
        }
    }
    return {best_index, best_value};
}
