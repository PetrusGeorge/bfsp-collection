#include "DE_PLS.h"

#include "Core.h"
#include "Log.h"
#include "RNG.h"
#include "constructions/GRASP_NEH.h"
#include "constructions/PF_NEH.h"
#include "local-search/RLS.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>

namespace {
size_t uptime() {
    static const auto global_start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - global_start_time);
    return duration.count();
}
} // namespace

DE_PLS::DE_PLS(Instance instance, Parameters params) : m_instance(std::move(instance)), m_params(std::move(params)), m_helper(m_instance) {
    if (auto tl = m_params.time_limit()) {
        m_time_limit = *tl;
    } else {
        this->m_time_limit = m_params.ro() * m_instance.num_jobs() * m_instance.num_machines();
    }
}

std::vector<size_t> DE_PLS::generate_random_sequence() {

    std::vector<size_t> v(m_instance.num_jobs());
    std::iota(v.begin(), v.end(), 0);
    std::shuffle(v.begin(), v.end(), RNG::instance().gen());

    return v;
}

bool DE_PLS::equal_solution(Solution &s1, Solution &s2) {

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

void DE_PLS::initialize_population() {

    PF_NEH pf_neh(m_instance);
    NEH neh(m_instance);
    GRASP_NEH grasp_neh(m_instance, m_params.delta(), m_params.beta());

    m_pop = std::vector<Solution>(m_params.np());

    // taking the first solution using PF-NEH
    // i think pf-neh is deterministic, so this loop is useless
    for (size_t i = 0; i < m_params.x(); i++) {
        const Solution pi = pf_neh.solve(m_params.delta(), i);
        if (pi.cost < m_pop[0].cost) {
            m_pop[0] = pi;
        }
    }

    // taking the second solution using GRASP-NEH
    for (size_t i = 0; i < m_params.x(); i++) {
        const Solution pi = grasp_neh.solve();
        if (pi.cost < m_pop[1].cost) {
            m_pop[1] = pi;
        }
    }
    core::recalculate_solution(m_instance, m_pop[1]);

    for (size_t i = 2; i < m_params.np(); i++) {
        const std::vector<size_t> phi = generate_random_sequence();
        m_pop[i] = neh.solve(phi);
        core::recalculate_solution(m_instance, m_pop[i]);
    }

    for (size_t i = 0; i < m_params.np(); i++) {
        m_pop[i].ds = RNG::instance().generate(m_params.ds_min(), m_params.ds_max());
        m_pop[i].ps = RNG::instance().generate(m_params.ps_min(), m_params.ps_max());
        m_pop[i].tau = RNG::instance().generate_real_number(m_params.tau_min(), m_params.tau_max());
        m_pop[i].jp = RNG::instance().generate_real_number(m_params.jp_min(), m_params.jp_max());
    }
}

std::vector<double> DE_PLS::get_mutant() {
    std::vector<double> x(4);

    // taking three random solutions
    const size_t ind_1 = RNG::instance().generate((size_t)0, m_pop.size() - 1);
    size_t ind_2 = ind_1;
    size_t ind_3 = ind_1;

    while (ind_2 == ind_1) {
        ind_2 = RNG::instance().generate((size_t)0, m_pop.size() - 1);
    }

    while (ind_3 == ind_1 || ind_3 == ind_2) {
        ind_3 = RNG::instance().generate((size_t)0, m_pop.size() - 1);
    }

    const size_t d = RNG::instance().generate((size_t)0, (size_t)3);

    if (RNG::instance().generate_real_number(0, 1) <= m_params.cr() || d == 0) {
        x[0] = m_pop[ind_1].ds + m_params.f() * (m_pop[ind_2].ds - m_pop[ind_3].ds);
    } else {
        x[0] = m_pop[ind_1].ds;
    }

    if (RNG::instance().generate_real_number(0, 1) <= m_params.cr() || d == 1) {
        x[1] = m_pop[ind_1].ps + m_params.f() * (m_pop[ind_2].ps - m_pop[ind_3].ps);
    } else {
        x[1] = m_pop[ind_1].ps;
    }

    if (RNG::instance().generate_real_number(0, 1) <= m_params.cr() || d == 2) {
        x[2] = m_pop[ind_1].tau + m_params.f() * (m_pop[ind_2].tau - m_pop[ind_3].tau);
    } else {
        x[2] = m_pop[ind_1].tau;
    }

    if (RNG::instance().generate_real_number(0, 1) <= m_params.cr() || d == 3) {
        x[2] = m_pop[ind_1].jp + m_params.f() * (m_pop[ind_2].jp - m_pop[ind_3].jp);
    } else {
        x[3] = m_pop[ind_1].jp;
    }

    return x;
}

void DE_PLS::get_trial(std::vector<double> &x) {
    if (x[0] < m_params.ds_min() || x[0] > m_params.ds_max()) {
        x[0] = (double)m_params.ds_min() +
               (m_params.ds_max() - m_params.ds_min()) * RNG::instance().generate_real_number(0, 1);
    }

    if (x[1] < m_params.ps_min() || x[1] > m_params.ps_max()) {
        x[1] = (double)m_params.ps_min() +
               (m_params.ps_max() - m_params.ps_min()) * RNG::instance().generate_real_number(0, 1);
    }

    if (x[2] < m_params.tau_min() || x[2] > m_params.tau_max()) {
        x[2] =
            m_params.tau_min() + (m_params.tau_max() - m_params.tau_min()) * RNG::instance().generate_real_number(0, 1);
    }

    if (x[3] < m_params.jp_min() || x[3] > m_params.jp_max()) {
        x[3] = m_params.jp_min() + (m_params.jp_max() - m_params.jp_min()) * RNG::instance().generate_real_number(0, 1);
    }
}

void DE_PLS::update_params(Solution &s, std::vector<double> &x) {

    s.ds = static_cast<size_t>(x[0]);
    s.ps = static_cast<size_t>(x[1]);
    s.tau = x[2];
    s.jp = x[3];
}

void DE_PLS::perturbation(Solution &s) {

    std::vector<size_t> jobs(s.ps);
    for (size_t i = 0; i < s.ps; i++) {
        const size_t shift = RNG::instance().generate((size_t)0, s.sequence.size() - 1);
        jobs[i] = s.sequence[shift];
        s.sequence.erase(s.sequence.begin() + shift);
    }

    for (size_t i = 0; i < s.ps; i++) {
        const size_t shift = RNG::instance().generate((size_t)0, s.sequence.size() - 1);
        s.sequence.insert(s.sequence.begin() + shift, jobs[i]);
    }
}

void DE_PLS::desconstruct_construct(Solution &s) {

    std::vector<size_t> jobs(s.ds);
    for (size_t i = 0; i < s.ds; i++) {
        const size_t shift = RNG::instance().generate((size_t)0, s.sequence.size() - 1);
        jobs[i] = s.sequence[shift];
        s.sequence.erase(s.sequence.begin() + shift);
    }

    m_helper.second_step(jobs, s);
}

Solution DE_PLS::solve() {

    const size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    std::vector<size_t> ro;
    if (m_params.benchmark()) {
        ro = {90, 60, 30};
    }

    const double multiplier = static_cast<double>(m_instance.total_processing_time()) / (10 * static_cast<double>(mxn));

    initialize_population();

    std::sort(m_pop.begin(), m_pop.end(), [](Solution &p1, Solution &p2) { return p1.cost < p2.cost; });

    Solution best_solution = m_pop[0];
    std::vector<size_t> ref = best_solution.sequence;
    
    while (true) {

        for (size_t i = 0; i < m_params.np(); i++) {

            Solution s = m_pop[i];

            std::vector<double> x = get_mutant();
            get_trial(x);
            update_params(s, x);

            if (RNG::instance().generate_real_number(0, 1) < s.jp) {
                perturbation(s);
            } else {
                desconstruct_construct(s);
            }

            core::recalculate_solution(m_instance, s);

            rls(s, ref, m_instance);
            core::recalculate_solution(m_instance, s);

            if (!ro.empty() && uptime() >= (ro.back() * mxn)) {
                std::cout << best_solution.cost << '\n';
                ro.pop_back();
            }

            if (uptime() > m_time_limit) {
                break;
            }

            if (s.cost <= m_pop[i].cost) {
                m_pop[i] = s;
                if (s.cost < best_solution.cost) {
                    best_solution = s;
                    ref = best_solution.sequence;
                }
            } else {
                const double delta = static_cast<double>(s.cost) - static_cast<double>(m_pop[i].cost);
                if (RNG::instance().generate_real_number(0, 1) < exp(-delta / (multiplier * s.tau))) {
                    m_pop[i] = s;
                }
            }
        }

        if (!ro.empty() && uptime() >= (ro.back() * mxn)) {

            std::cout << best_solution.cost << '\n';
            ro.pop_back();
        }

        if (uptime() > m_time_limit) {
            break;
        }
    }

    return best_solution;
}