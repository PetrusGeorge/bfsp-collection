#include "HDDE.h"
#include "local-search/RLS.h"

#include <algorithm>
#include <chrono>
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

HDDE::HDDE(Instance instance, Parameters params)
    : m_instance(std::move(instance)), m_params(std::move(params)),
      m_time_limit((m_params.ro() * m_instance.num_jobs() * m_instance.num_machines()) / 1000) {}

bool HDDE::new_in_population(std::vector<size_t> &sequence) {

    // check every solution in population
    for (auto &i : m_pop) {

        bool already_exist = true;
        for (size_t j = 0; j < i.sequence.size(); j++) {

            if (sequence[j] != i.sequence[j]) {
                already_exist = false;
                break;
            }
        }

        if (already_exist) {
            return false;
        }
    }

    return true;
}

void HDDE::generate_initial_pop() {

    m_pop = std::vector<Solution>(1);

    const Solution phi = LPT::solve(m_instance);

    NEH neh = NEH(m_instance);
    m_pop[0] = neh.solve(phi.sequence);
    core::recalculate_solution(m_instance, m_pop[0]);

    // generating other random solutions
    for (size_t i = 1; i < m_params.ps(); i++) {
        Solution s;
        s.sequence = generate_random_sequence();
        m_pop.push_back(s);
        core::recalculate_solution(m_instance, m_pop[i]);
    }
}

std::vector<size_t> HDDE::generate_random_sequence() {

    std::vector<size_t> v(m_instance.num_jobs());
    std::iota(v.begin(), v.end(), 0);
    std::shuffle(v.begin(), v.end(), RNG::instance().gen());

    return v;
}

std::vector<size_t> HDDE::mutation() {
    const size_t n = m_instance.num_jobs();

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

    std::vector<size_t> new_pi(n);
    for (size_t i = 0; i < n; i++) {

        // use the formula given in the article to generate a(n possibly invalid) new solution
        if (RNG::instance().generate_real_number(0, 1) < m_params.z()) {
            new_pi[i] = (m_pop[ind_1].sequence[i] + n + 1 + m_pop[ind_2].sequence[i] - m_pop[ind_3].sequence[i]) % n;
        } else {
            new_pi[i] = (m_pop[ind_1].sequence[i] + n + 1) % n;
        }
    }

    return new_pi;
}

Solution HDDE::crossover(std::vector<size_t> &pi) {
    const size_t n = m_instance.num_jobs();
    std::vector<size_t> pi_temp;

    // putting some unique jobs into pi_temp
    for (size_t i = 0; i < n; i++) {
        if (RNG::instance().generate_real_number(0, 1) > m_params.cr()) {
            continue;
        }

        bool out_pi_temp = true;
        for (const unsigned long j : pi_temp) {
            if (pi[i] == j) {
                out_pi_temp = false;
                break;
            }
        }

        if (out_pi_temp) {
            pi_temp.push_back(pi[i]);
        }
    }

    // getting any solution from population as a reference
    std::vector<size_t> ref = m_pop[RNG::instance().generate((size_t)0, m_pop.size() - 1)].sequence;

    // finding the position of pi_temp jobs jobs within ref
    std::vector<size_t> deleted;
    for (size_t i = ref.size() - 1; i < ref.size(); --i) {

        for (const unsigned long j : pi_temp) {
            if (ref[i] == j) {
                deleted.push_back(i);
                break;
            }
        }

        if (pi_temp.size() == deleted.size()) {
            break;
        }
    }

    // deleting ref jobs are in pi_temp
    for (const unsigned long i : deleted) {
        ref.erase(ref.begin() + static_cast<long>(i));
    }

    // neh second step in ref to make ref have all the jobs missing in the best position
    Solution s;
    s.sequence.swap(ref);

    NEH neh = NEH(m_instance);
    neh.second_step(pi_temp, s);

    return s;
}

Solution HDDE::generate_new_solution() {
    Solution s;
    std::vector<size_t> new_seq = mutation();
    s = crossover(new_seq);

    core::recalculate_solution(m_instance, s);

    return s;
}

size_t HDDE::find_best_solution() {
    size_t best = 0;
    for (size_t i = 1; i < m_pop.size(); i++) {
        if (m_pop[i].cost < m_pop[best].cost) {
            best = i;
        }
    }

    return best;
}

Solution HDDE::solve() {

    const size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    std::vector<size_t> ro;
    if (m_params.benchmark()) {
        ro = {90, 60, 30};
    }

    Solution best_solution;
    size_t idx = 0;

    generate_initial_pop();

    idx = find_best_solution();

    best_solution = m_pop[idx];

    std::vector<size_t> ref(m_instance.num_jobs());
    std::iota(ref.begin(), ref.end(), 0);

    while (true) {
        for (auto &i : m_pop) {
            Solution trial = generate_new_solution();

            std::shuffle(ref.begin(), ref.end(), RNG::instance().gen());

            rls_grabowski(trial, ref, m_instance);

            if (trial.cost < i.cost) {
                i = trial;
            }
        }

        idx = find_best_solution();

        if (!ro.empty() && uptime() >= ro.back() * mxn / 1000) {
            std::cout << best_solution.cost << '\n';
            ro.pop_back();
        }

        if (uptime() > m_time_limit) {
            break;
        }

        if (m_pop[idx].cost < best_solution.cost) {
            best_solution = m_pop[idx];
        }
    }

    m_pop.clear();
    return best_solution;
}
