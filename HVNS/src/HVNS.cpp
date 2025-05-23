#include "HVNS.h"

#include <algorithm>
#include <chrono>
#include <cmath>
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

HVNS::HVNS(Instance instance, Parameters params) : m_instance(std::move(instance)), m_params(std::move(params)) {
    if (auto tl = m_params.time_limit()) {
        m_time_limit = *tl;
    } else {
        this->m_time_limit = m_params.ro() * m_instance.num_jobs() * m_instance.num_machines() / 1000;
    }

    size_t processing_times_sum = 0;
    std::vector<size_t> vec_processing_times_sum = m_instance.processing_times_sum();

    for (unsigned long i : vec_processing_times_sum) {
        processing_times_sum += i;
    }

    m_T_init = static_cast<double>(processing_times_sum) / (m_instance.num_jobs() * m_instance.num_machines() * 5);
    m_T = m_T_init;
    m_T_fin = m_T_init * 0.1;
    m_beta = (m_T_init - m_T_fin) / (m_params.n_iter() * m_T_init * m_T_fin);
}

Solution HVNS::generate_first_solution() {

    const std::vector<size_t> sorted_sequece = core::stpt_sort(m_instance);

    NEH neh(m_instance);
    Solution s = neh.solve(sorted_sequece);

    return s;
}

std::vector<size_t> HVNS::generate_random_sequence() {

    std::vector<size_t> v(m_instance.num_jobs());
    std::iota(v.begin(), v.end(), 0);
    std::shuffle(v.begin(), v.end(), RNG::instance().gen());

    return v;
}

bool HVNS::equal_solution(Solution &s1, Solution &s2) {

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

std::pair<size_t, size_t> HVNS::taillard_best_edge_insertion(const std::vector<size_t> &sequence,
                                                             std::pair<size_t, size_t> &jobs,
                                                             size_t original_position) {
    const size_t m = m_instance.num_machines();

    auto m_e = core::calculate_departure_times(m_instance, sequence);

    auto m_q = core::calculate_tail(m_instance, sequence);
    // Make it easier to implement find_best_insertion
    // without an out of bound access
    m_q.emplace_back(m_instance.num_machines(), 0);

    auto p = [this](size_t i, size_t j) { return m_instance.p(i, j); };

    auto m_f = std::vector(sequence.size() + 1, std::vector<size_t>(m_instance.num_machines()));

    m_f[0][0] = p(jobs.first, 0);
    for (size_t j = 1; j < m; j++) {
        m_f[0][j] = m_f[0][j - 1] + p(jobs.first, j);
    }

    for (size_t i = 1; i < m_f.size(); i++) {

        m_f[i][0] = std::max(m_e[i - 1][0] + p(jobs.first, 0), m_e[i - 1][1]);
        size_t j = 0;
        for (j = 1; j < m_f[i].size() - 1; j++) {
            m_f[i][j] = std::max(m_f[i][j - 1] + p(jobs.first, j), m_e[i - 1][j + 1]);
        }
        m_f[i][j] = m_f[i][j - 1] + p(jobs.first, j);
    }

    // Evaluate best insertion
    size_t max_value = 0;
    auto set_f_and_max = [&m_f, &m_q, &max_value](size_t i, size_t j, size_t value) {
        m_f[i][j] = value;
        max_value = std::max(value + m_q[i][j], max_value);
    };

    size_t best_index = 0;
    size_t best_value = 0;
    if (original_position != 0) {
        set_f_and_max(0, 0, std::max(m_f[0][0] + p(jobs.second, 0), m_f[0][1]));
        size_t j = 0;
        for (j = 1; j < m_instance.num_machines() - 1; j++) {
            set_f_and_max(0, j, std::max(m_f[0][j - 1] + p(jobs.second, j), m_f[0][j + 1]));
        }
        set_f_and_max(0, j, m_f[0][j - 1] + p(jobs.second, j));

        best_index = 0;
        best_value = max_value;
    } else {
        best_index = 1;
        best_value = std::numeric_limits<size_t>::max();
    }

    for (size_t i = 1; i <= sequence.size(); i++) {
        if (original_position == i) {
            continue;
        }

        max_value = 0;
        size_t value = std::max(m_f[i][0] + p(jobs.second, 0), m_f[i][1]);
        set_f_and_max(i, 0, value);

        for (size_t j = 1; j < m_instance.num_machines() - 1; j++) {
            value = std::max(m_f[i][j - 1] + p(jobs.second, j), m_f[i][j + 1]);
            set_f_and_max(i, j, value);
        }
        value = m_f[i][m_instance.num_machines() - 2] + p(jobs.second, m_instance.num_machines() - 1);
        set_f_and_max(i, m_instance.num_machines() - 1, value);

        if (max_value < best_value) {
            best_value = max_value;
            best_index = i;
        }
    }

    return {best_index, best_value};
}

void HVNS::best_insertion(Solution &s) {

    NEH helper(m_instance);
    size_t best_job_index = 0;
    size_t best_obj = s.cost;
    size_t best_index = std::numeric_limits<size_t>::max();

    for (size_t i = 0; i < m_instance.num_jobs(); i++) {

        const size_t job = s.sequence[i];
        s.sequence.erase(s.sequence.begin() + (long)i);

        auto [index, obj] = helper.taillard_best_insertion(s.sequence, job, std::numeric_limits<size_t>::max());

        s.sequence.insert(s.sequence.begin() + (long)i, job);

        if (obj < best_obj) {
            best_obj = obj;
            best_index = index;
            best_job_index = i;
        }
    }

    if (s.cost == best_obj) {
        return;
    }

    if (best_job_index < best_index) {
        std::rotate(s.sequence.begin() + best_job_index, s.sequence.begin() + best_job_index + 1,
                    s.sequence.begin() + best_index);
    } else {
        std::rotate(s.sequence.begin() + best_index, s.sequence.begin() + best_job_index,
                    s.sequence.begin() + best_job_index + 1);
    }

    core::recalculate_solution(m_instance, s);
}

void HVNS::best_edge_insertion(Solution &s) {

    size_t best_job = 0;
    size_t best_index = std::numeric_limits<size_t>::max();
    size_t best_obj = s.cost;

    for (size_t i = 0; i < m_instance.num_jobs() - 1; i++) {

        std::pair<size_t, size_t> jobs = {s.sequence[i], s.sequence[i + 1]};
        s.sequence.erase(s.sequence.begin() + (long)i, s.sequence.begin() + (long)i + 2);

        auto [index, obj] = taillard_best_edge_insertion(s.sequence, jobs, std::numeric_limits<size_t>::max());

        s.sequence.insert(s.sequence.begin() + (long)i, jobs.first);
        s.sequence.insert(s.sequence.begin() + (long)i + 1, jobs.second);

        if (obj < best_obj) {
            best_obj = obj;
            best_index = index;
            best_job = i;
        }
    }

    if (s.cost == best_obj) {
        return;
    }

    if (best_job < best_index) {
        std::rotate(s.sequence.begin() + best_job, s.sequence.begin() + best_job + 2,
                    s.sequence.begin() + best_index + 2);
    } else {
        std::rotate(s.sequence.begin() + best_index, s.sequence.begin() + best_job, s.sequence.begin() + best_job + 2);
    }

    core::recalculate_solution(m_instance, s);
}

void HVNS::best_swap(Solution &s) {

    Solution copy = s;
    size_t best_i = 0;
    size_t best_j = 0;
    size_t best_obj = s.cost;

    for (size_t i = 0; i < m_instance.num_jobs(); i++) {

        for (size_t j = i + 1; j < m_instance.num_jobs(); j++) {

            std::swap(copy.sequence[i], copy.sequence[j]);

            core::partial_recalculate_solution(m_instance, copy, i);

            std::swap(copy.sequence[i], copy.sequence[j]);

            if (copy.cost < best_obj) {
                best_obj = copy.cost;
                best_i = i;
                best_j = j;
            }
        }

        copy.departure_times[i] = s.departure_times[i];
    }

    if (s.cost == best_obj) {
        return;
    }

    std::swap(s.sequence[best_i], s.sequence[best_j]);

    core::partial_recalculate_solution(m_instance, s, best_i);
}

void HVNS::shaking(Solution &s, size_t k) {

    Solution copy = s;
    switch (k) {
    case 1:
        best_insertion(copy);
        break;
    case 2:
        best_edge_insertion(copy);
        break;
    case 3:
        best_swap(copy);
        break;
    }

    if (copy.cost < s.cost) {
        s = copy;
    }
}

void HVNS::sa_rls(Solution &current, Solution &best) {

    std::vector<size_t> ref = generate_random_sequence();
    size_t cnt = 0;
    NEH helper(m_instance);
    while (cnt < m_instance.num_jobs()) {

        const size_t job = ref[cnt];
        size_t i = 0;
        for (i = 0; i < current.sequence.size(); i++) {
            if (current.sequence[i] == job) {
                current.sequence.erase(current.sequence.begin() + (long)i);
                break;
            }
        }

        auto [best_index, best_nei_obj] = helper.taillard_best_insertion(current.sequence, job, i);

        const double delta = static_cast<double>(current.cost) - static_cast<double>(best_nei_obj);

        if (delta != 0 &&
            (!(best_nei_obj > current.cost) || RNG::instance().generate_real_number(0, 1) < exp(delta / m_T))) {
            current.sequence.insert(current.sequence.begin() + best_index, job);
            current.cost = best_nei_obj;
            std::shuffle(ref.begin(), ref.end(), RNG::instance().gen());
            cnt = 0;
        } else {
            current.sequence.insert(current.sequence.begin() + (long)i, job);
            cnt++;
        }

        if (best_nei_obj < best.cost) {
            best = current;
        }

        m_T = m_T / (1 + (m_beta * m_T));
    }
}

void HVNS::sa_best_edge_insertion(Solution &current, Solution &best) {

    std::vector<size_t> ref = generate_random_sequence();
    size_t cnt = 0;
    NEH const helper(m_instance);
    while (cnt < m_instance.num_jobs()) {
        std::pair<size_t, size_t> jobs;
        jobs.first = ref[cnt];

        size_t i = 0;
        for (i = 0; i < current.sequence.size() - 1; i++) {
            if (current.sequence[i] == jobs.first) {
                jobs.second = current.sequence[i + 1];
                current.sequence.erase(current.sequence.begin() + i, current.sequence.begin() + i + 2);
                break;
            }
        }

        if (i == m_instance.num_jobs() - 1) {
            cnt++;
            continue;
        }

        auto [best_index, best_nei_obj] = taillard_best_edge_insertion(current.sequence, jobs, i);

        const double delta = static_cast<double>(current.cost) - static_cast<double>(best_nei_obj);

        if (delta != 0 &&
            (!(best_nei_obj > current.cost) || RNG::instance().generate_real_number(0, 1) < exp(delta / m_T))) {
            current.sequence.insert(current.sequence.begin() + best_index, jobs.first);
            current.sequence.insert(current.sequence.begin() + best_index + 1, jobs.second);
            current.cost = best_nei_obj;
            std::shuffle(ref.begin(), ref.end(), RNG::instance().gen());
            cnt = 0;
        } else {
            current.sequence.insert(current.sequence.begin() + i, jobs.first);
            current.sequence.insert(current.sequence.begin() + i + 1, jobs.second);
            cnt++;
        }

        if (best_nei_obj < best.cost) {
            best = current;
        }

        m_T = m_T / (1 + (m_beta * m_T));
    }
}

Solution HVNS::solve() {

    const size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    std::vector<size_t> ro;
    if (m_params.benchmark()) {
        ro = {90, 60, 30};
    }

    uptime();

    Solution best_solution;
    Solution current;

    best_solution = current = generate_first_solution();

    size_t previous_best_obj = best_solution.cost;
    while (true) {
        size_t k = 1;

        while (k <= KMAX) {

            shaking(current, k);

            if (!ro.empty() && uptime() >= (ro.back() * mxn) / 1000) {

                std::cout << best_solution.cost << '\n';
                ro.pop_back();
            }

            if (uptime() > m_time_limit) {
                break;
            }

            if (current.cost < best_solution.cost) {
                best_solution = current;
            }

            Solution temp;
            do {
                sa_rls(current, best_solution);

                temp.sequence = current.sequence;
                temp.cost = current.cost;

                sa_best_edge_insertion(current, best_solution);

                if (!ro.empty() && uptime() >= (ro.back() * mxn) / 1000) {

                    std::cout << best_solution.cost << '\n';
                    ro.pop_back();
                }

                if (uptime() > m_time_limit) {
                    break;
                }

            } while (!equal_solution(current, temp));

            if (uptime() > m_time_limit) {
                break;
            }

            core::recalculate_solution(m_instance, current);
            core::recalculate_solution(m_instance, best_solution);

            if (best_solution.cost < previous_best_obj) {
                k = 1;
                previous_best_obj = best_solution.cost;
            } else {
                k++;
            }
        }

        if (!ro.empty() && uptime() >= (ro.back() * mxn) / 1000) {

            std::cout << best_solution.cost << '\n';
            ro.pop_back();
        }

        if (uptime() > m_time_limit) {
            break;
        }
    }

    core::recalculate_solution(m_instance, best_solution);

    return best_solution;
}