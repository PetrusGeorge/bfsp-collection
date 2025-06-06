#include "constructions/NEH.h"

#include "Core.h"
#include "Instance.h"
#include "Solution.h"
#include <algorithm>
#include <limits>
#include <utility>

NEH::NEH(Instance &instance) : m_instance(instance) {

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
    size_t old = 0;

    auto p = [this](size_t i, size_t j) { return m_instance.p(i, j); };

    auto &q = m_inner.tail;
    auto &e = m_inner.departure_times;

    auto set_old_and_max = [&old, &max_value, &q](size_t i, size_t j, size_t value) {
        max_value = std::max(value + q[i][j], max_value);
        old = value;
    };

    size_t value = std::max(e[i - 1][0] + p(k, 0), e[i - 1][1]);
    set_old_and_max(i, 0, value);

    if (max_value >= best_value) {
        return max_value;
    }

    for (size_t j = 1; j < m_instance.num_machines() - 1; j++) {
        value = std::max(old + p(k, j), e[i - 1][j + 1]);
        set_old_and_max(i, j, value);

        if (max_value >= best_value) {
            return max_value;
        }
    }

    value = old + p(k, m_instance.num_machines() - 1);
    set_old_and_max(i, m_instance.num_machines() - 1, value);

    return max_value;
}

std::pair<size_t, size_t> NEH::taillard_best_insertion(const std::vector<size_t> &s, size_t k) {
    m_inner.sequence = s;

    core::calculate_departure_times(m_instance, m_inner);
    core::calculate_tail(m_instance, m_inner);

    auto &q = m_inner.tail;

    auto p = [this](size_t i, size_t j) { return m_instance.p(i, j); };

    // Evaluate best insertion
    size_t max_value = 0;
    size_t old = 0;
    auto set_old_and_max = [&old, &max_value, &q](size_t i, size_t j, size_t value) {
        max_value = std::max(value + q[i][j], max_value);
        old = value;
    };

    set_old_and_max(0, 0, p(k, 0));
    for (size_t j = 1; j < m_instance.num_machines(); j++) {
        set_old_and_max(0, j, old + p(k, j));
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

std::pair<size_t, size_t> NEH::mtaillard_best_insertion(const std::vector<size_t> &s, size_t k, size_t original_position) {
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
    size_t old = 0;
    auto set_old_and_max = [&old, &max_value, &q](size_t i, size_t j, size_t value) {
        max_value = std::max(value + q[i][j], max_value);
        old = value;
    };

    set_old_and_max(0, 0, p(k, 0));
    for (size_t j = 1; j < m_instance.num_machines(); j++) {
        set_old_and_max(0, j, old + p(k, j));
    }

    size_t best_index = 0;
    size_t best_value = std::numeric_limits<size_t>::max();
    
    if (original_position != 0) {
        set_old_and_max(0, 0, p(k, 0));
        for (size_t j = 1; j < m_instance.num_machines(); j++) {
            set_old_and_max(0, j, old + p(k, j));
        }
        best_index = 0;
        best_value = max_value;
    } else {
        best_index = 1;
        best_value = std::numeric_limits<size_t>::max();
    }

    for (size_t i = 1; i < original_position; i++) {
        max_value = insert_calculation(i, k, best_value);

        if (max_value < best_value) {
            best_value = max_value;
            best_index = i;
        }
    }

    for (size_t i = original_position+1; i <= s.size(); i++) {
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
