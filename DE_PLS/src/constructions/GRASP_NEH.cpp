#include "constructions/GRASP_NEH.h"
#include "Core.h"
#include "RNG.h"
#include "constructions/NEH.h"
#include <iostream>

#include <bits/types/cookie_io_functions_t.h>
#include <cassert>
#include <numeric>

GRASP_NEH::GRASP_NEH(Instance &instance, size_t x, double beta) : m_instance(instance), m_x(x), m_beta(beta) {}

Solution GRASP_NEH::solve() {
    Solution pi_h = GRASP();

    const std::vector<size_t> phi = {pi_h.sequence.begin() + m_x, pi_h.sequence.end()};

    pi_h.sequence.erase(pi_h.sequence.begin() + m_x, pi_h.sequence.end());

    NEH helper(m_instance);

    helper.second_step(phi, pi_h);

    core::recalculate_solution(m_instance, pi_h);

    return pi_h;
}

Solution GRASP_NEH::GRASP() {
    Solution s;
    std::vector<size_t> dp(m_instance.num_machines(), 0);
    auto jobs = core::stpt_sort(m_instance);

    std::vector<bool> b_s(m_instance.num_jobs(), false);

    s.sequence.push_back(jobs[0]);
    b_s[0] = true;

    for (size_t i = 1; i < m_instance.num_jobs(); i++) {
        core::recalculate_solution(m_instance, s);
        std::vector<size_t> rcl;

        std::vector<size_t> c(m_instance.num_jobs());
        size_t c_min = std::numeric_limits<size_t>::max();
        size_t c_max = 0;
        for (size_t j = 0; j < m_instance.num_jobs(); j++) {
            if (b_s[j]) {
                continue;
            }

            core::calculate_new_departure_time(m_instance, s.departure_times, dp, jobs[j]);
            c[j] = core::calculate_sigma(m_instance, s.departure_times, dp, jobs[j]);
            c_max = std::max(c[j], c_max);
            c_min = std::min(c[j], c_min);
        }

        const double accept_criterion = static_cast<double>(c_min) + m_beta * static_cast<double>((c_max - c_min));

        for (size_t j = 0; j < m_instance.num_jobs(); j++) {
            if (b_s[j]) {
                continue;
            }

            if (c[j] <= accept_criterion) {
                rcl.push_back(j);
            }
        }

        const size_t idx = RNG::instance().generate((size_t)0, rcl.size() - 1);
        s.sequence.push_back(jobs[rcl[idx]]);
        b_s[rcl[idx]] = true;
    }

    return s;
}
