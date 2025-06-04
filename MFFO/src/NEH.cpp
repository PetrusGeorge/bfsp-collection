#include "NEH.h"

#include "Core.h"
#include "Instance.h"
#include "Solution.h"
#include <algorithm>
#include <utility>

NEH::NEH(Instance &instance) : m_instance(instance) {
    
    // f needs to store all possibilities of insertion so it has s.sequence.size + 1
    m_f = std::vector(instance.num_jobs() + 1, std::vector<size_t>(m_instance.num_machines()));
    m_inner.departure_times = std::vector(instance.num_jobs(), std::vector<size_t>(instance.num_machines()));
    m_inner.tail = std::vector(instance.num_jobs()+1, std::vector<size_t>(instance.num_machines()));
}

Solution NEH::solve(std::vector<size_t> phi) {

    Solution s;
    s.sequence.reserve(m_instance.num_jobs());
    s.sequence = {phi.front()};
    phi.erase(phi.begin());


    second_step(std::move(phi), s);
    return s;
}

size_t NEH::insert_calculation(const size_t i, const size_t k, const size_t best_value) {
    size_t max_value = 0;

    auto p = [this](size_t i, size_t j) { return m_instance.p(i, j); };

    auto &q = m_inner.tail;
    auto &e = m_inner.departure_times;
    
    auto set_f_and_max = [this, &max_value, &q](size_t i, size_t j, size_t value) {
        m_f[i][j] = value;
        max_value = std::max(value + q[i][j], max_value);
    };

    size_t value = std::max(e[i - 1][0] + p(k, 0), e[i - 1][1]);
    set_f_and_max(i, 0, value);

    if (max_value >= best_value) {
        return max_value;
    }

    for (size_t j = 1; j < m_instance.num_machines() - 1; j++) {
        value = std::max(m_f[i][j - 1] + p(k, j), e[i - 1][j + 1]);
        set_f_and_max(i, j, value);

        if (max_value >= best_value) {
            return max_value;
        }
    }

    value = m_f[i][m_instance.num_machines() - 2] + p(k, m_instance.num_machines() - 1);
    set_f_and_max(i, m_instance.num_machines() - 1, value);

    return max_value;
}

std::pair<size_t, size_t> NEH::taillard_grabowski_best_ins(const Solution &s, const size_t k,
                                                           const std::vector<std::pair<size_t, size_t>> &ranges) {

    m_inner.sequence = s.sequence;
    // const std::vector<size_t> &sequence = s.sequence;
    
    core::calculate_departure_times(m_instance, m_inner);

    core::calculate_tail(m_instance, m_inner);
    // Make it easier to implement find_best_insertion
    // without an out of bound access
    // m_q.emplace_back(m_instance.num_machines(), 0);

    auto p = [this](size_t i, size_t j) { return m_instance.p(i, j); };

    // f needs to store all possibilities of insertion so it has sequence.size + 1
    m_f = std::vector(m_inner.sequence.size() + 1, std::vector<size_t>(m_instance.num_machines()));

    // Evaluate best insertion
    size_t max_value = 0;

    if (ranges[0].first == 0) {
        auto set_f_and_max = [this, &max_value](size_t i, size_t j, size_t value) {
            m_f[i][j] = value;
            max_value = std::max(value + m_inner.tail[i][j], max_value);
        };

        set_f_and_max(0, 0, p(k, 0));
        for (size_t j = 1; j < m_instance.num_machines(); j++) {
            set_f_and_max(0, j, m_f[0][j - 1] + p(k, j));
        }
    }

    size_t best_index = 0;
    size_t best_value = std::min(max_value, s.cost);

    for (const auto &range : ranges) {
        for (size_t i = range.first; i <= range.second; i++) {
            if (i == 0) {
                continue;
            }

            max_value = insert_calculation(i, k, best_value);

            if (max_value < best_value) {
                best_value = max_value;
                best_index = i;
            }
        }
    }

    return {best_index, best_value};
}

std::pair<size_t, size_t> NEH::taillard_best_insertion(const std::vector<size_t> &s, size_t k) {
    m_inner.sequence = s;

    core::calculate_departure_times(m_instance, m_inner);
    core::calculate_tail(m_instance, m_inner);
    // Make it easier to implement find_best_insertion
    // without an out of bound access
    // inner.tail.emplace_back(m_instance.num_machines(), 0);

    auto &q = m_inner.tail;

    auto p = [this](size_t i, size_t j) { return m_instance.p(i, j); };

    // Evaluate best insertion
    size_t max_value = 0;
    auto set_f_and_max = [this, &max_value, &q](size_t i, size_t j, size_t value) {
        m_f[i][j] = value;
        max_value = std::max(value + q[i][j], max_value);
    };

    set_f_and_max(0, 0, p(k, 0));
    for (size_t j = 1; j < m_instance.num_machines(); j++) {
        set_f_and_max(0, j, m_f[0][j - 1] + p(k, j));
    }

    size_t best_index = 0;
    size_t best_value = max_value;

    for (size_t i = 1; i <= s.size(); i++) {
        max_value = insert_calculation(i, k, best_value);

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
