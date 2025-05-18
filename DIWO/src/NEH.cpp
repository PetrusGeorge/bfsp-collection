#include "NEH.h"

#include "Core.h"
#include "Instance.h"
#include "Solution.h"
#include <utility>

NEH::NEH(Instance &instance) : m_instance(instance) {}

Solution NEH::solve(std::vector<size_t> phi) {
    Solution s;
    s.sequence.reserve(m_instance.num_jobs());
    s.sequence = {phi[0]};
    phi.erase(phi.begin());

    second_step(phi, s);
    return s;
}

std::pair<size_t, size_t> NEH::taillard_best_insertion(const std::vector<size_t> &sequence, size_t pos) {
    m_e = core::calculate_departure_times(m_instance, sequence);

    m_q = core::calculate_tail(m_instance, sequence);
    // Make it easier to implement find_best_insertion
    // without an out of bound access
    m_q.emplace_back(m_instance.num_machines(), 0);

    auto p = [this](size_t i, size_t j) { return m_instance.p(i, j); };

    // f needs to store all possibilities of insertion so it has sequence.size + 1
    m_f = std::vector(sequence.size() + 1, std::vector<size_t>(m_instance.num_machines()));

    // Evaluate best insertion
    size_t max_value = 0;
    auto set_f_and_max = [this, &max_value](size_t i, size_t j, size_t value) {
        m_f[i][j] = value;
        max_value = std::max(value + m_q[i][j], max_value);
    };

    set_f_and_max(0, 0, p(pos, 0));
    for (size_t j = 1; j < m_instance.num_machines(); j++) {
        set_f_and_max(0, j, m_f[0][j - 1] + p(pos, j));
    }

    size_t best_index = 0;
    size_t best_value = max_value;

    for (size_t i = 1; i <= sequence.size(); i++) {
        max_value = 0;
        size_t value = std::max(m_e[i - 1][0] + p(pos, 0), m_e[i - 1][1]);
        set_f_and_max(i, 0, value);

        for (size_t j = 1; j < m_instance.num_machines() - 1; j++) {
            value = std::max(m_f[i][j - 1] + p(pos, j), m_e[i - 1][j + 1]);
            set_f_and_max(i, j, value);
        }
        value = m_f[i][m_instance.num_machines() - 2] + p(pos, m_instance.num_machines() - 1);
        set_f_and_max(i, m_instance.num_machines() - 1, value);

        if (max_value < best_value) {
            best_value = max_value;
            best_index = i;
        }
    }

    return {best_index, best_value};
}

void NEH::second_step(std::vector<size_t> phi, Solution &s) {

    while (!phi.empty()) {
        auto [best_index, makespan] = taillard_best_insertion(s.sequence, phi.front());

        s.sequence.insert(s.sequence.begin() + (long)best_index, phi.front());
        s.cost = makespan;

        phi.erase(phi.begin());
    }
}
