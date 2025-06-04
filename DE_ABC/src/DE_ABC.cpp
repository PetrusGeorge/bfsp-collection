#include "DE_ABC.h"

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

DE_ABC::DE_ABC(Instance instance, Parameters params) : m_instance(std::move(instance)), m_params(std::move(params)), helper(m_instance) {
    if (auto tl = m_params.tl()) {
        this->m_time_limit = *tl;
    } else {
        this->m_time_limit = m_params.ro() * m_instance.num_jobs() * m_instance.num_machines() / 1000;
    }

    // initializing the neighborhood list
    for (size_t i = 0; i < m_params.ps(); i++) {
        m_NL.push_back(RNG::instance().generate((size_t)0, (size_t)3));
    }

    m_changed = std::vector<bool>(m_params.ps(), false);
}

bool DE_ABC::new_in_population(std::vector<size_t> &sequence) {

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

void DE_ABC::generate_initial_pop() {

    m_pop = std::vector<Solution>(1);
    MinMax mm = MinMax(m_instance, m_params.theta());
    m_pop[0] = helper.solve(mm.solve().sequence); // MME heuristic for the first solution
    core::recalculate_solution(m_instance, m_pop[0]);

    std::vector<size_t> new_seq(m_instance.num_jobs());
    std::iota(new_seq.begin(), new_seq.end(), 0);

    // generating other random solutions
    while (m_pop.size() < m_params.ps()) {
        
        std::shuffle(new_seq.begin(), new_seq.end(), RNG::instance().gen());

        if (!new_in_population(new_seq)) {
            continue;
        }

        Solution s;
        s.sequence = new_seq;
        core::recalculate_solution(m_instance, s);
        m_pop.push_back(std::move(s));
    }
}

size_t DE_ABC::tournament() {

    const size_t i = RNG::instance().generate((size_t)0, m_pop.size() - 1);
    size_t j = i;

    while (i == j) {
        j = RNG::instance().generate((size_t)0, m_pop.size() - 1);
    }

    if (m_pop[i].cost > m_pop[j].cost) {
        return j;
    }
    return i;
}

std::vector<size_t> DE_ABC::mutation() {
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
        if (RNG::instance().generate_real_number(0, 1) < m_params.pmu()) {
            new_pi[i] = (m_pop[ind_1].sequence[i] + n + 1 + m_pop[ind_2].sequence[i] - m_pop[ind_3].sequence[i]) % n;
        } else {
            new_pi[i] = (m_pop[ind_1].sequence[i] + n + 1) % n;
        }
    }

    return new_pi;
}

Solution DE_ABC::crossover(std::vector<size_t> &pi) {
    const size_t n = m_instance.num_jobs();
    std::vector<size_t> pi_temp;

    // putting some unique jobs into pi_temp
    for (size_t i = 0; i < n; i++) {
        if (RNG::instance().generate_real_number(0, 1) < m_params.pc()) {
            continue;
        }

        bool out_pi_temp = true;
        for (unsigned long j : pi_temp) {
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
    size_t k = 0;
    for (size_t i = ref.size() - 1; i < ref.size(); --i) {

        for (unsigned long j : pi_temp) {
            if (ref[i] == j) {
                deleted.push_back(i);
                k++;
                break;
            }
        }

        if (pi_temp.size() == deleted.size()) {
            break;
        }
    }

    // deleting ref jobs are in pi_temp
    for (unsigned long i : deleted) {
        ref.erase(ref.begin() + i);
    }

    // neh second step in ref to make ref have all the jobs missing in the best position
    Solution s;
    s.sequence.swap(ref);

    helper.second_step(pi_temp, s);

    return s;
}

Solution DE_ABC::generate_new_solution() {
    Solution s;
    std::vector<size_t> new_seq = mutation();
    s = crossover(new_seq);

    core::recalculate_solution(m_instance, s);

    return s;
}

void DE_ABC::update_neighborhood() {
    m_BNL.resize(m_BNL.size() * 0.7);
    m_NL.swap(m_BNL);
    m_BNL.clear();

    for (size_t i = m_NL.size(); i < m_params.ps(); i++) {
        m_NL.push_back(RNG::instance().generate((size_t)0, (size_t)3));
    }
}

size_t DE_ABC::swap(Solution &s) {
    const size_t n = m_instance.num_jobs();
    const size_t idx_1 = RNG::instance().generate((size_t)0, n - 2);
    const size_t idx_2 = RNG::instance().generate(idx_1 + 1, n - 1);

    std::swap(s.sequence[idx_1], s.sequence[idx_2]);

    return idx_1;
}

size_t DE_ABC::insertion(Solution &s) {
    const size_t n = m_instance.num_jobs();
    const size_t idx_1 = RNG::instance().generate((size_t)0, n - 3);
    const size_t idx_2 = RNG::instance().generate(idx_1 + 2, n - 1);

    std::rotate(s.sequence.begin() + idx_1 + 1, s.sequence.begin() + idx_2, s.sequence.begin() + idx_2 + 1);

    return idx_1;
}

void DE_ABC::self_adaptative() {
    size_t idx = 0;
    size_t idx_to_recalculate = 0;
    for (size_t i = 0; i < m_params.ps(); i++) {
        idx = tournament();

        Solution s = m_pop[idx];

        switch (m_NL[i]) {
        case 0:
            idx_to_recalculate = insertion(s);
            break;
        case 1:
            idx_to_recalculate = swap(s);
            break;
        case 2:
            idx_to_recalculate = std::min(insertion(s), insertion(s));
            break;
        case 3:
            idx_to_recalculate = std::min(swap(s), swap(s));
        }

        core::partial_recalculate_solution(m_instance, s, idx_to_recalculate);

        if (s.cost < 1280) {
            std::cout << i << " " << m_NL[i] << " " << idx_to_recalculate << std::endl;
        }

        if (s.cost < m_pop[idx].cost) {
            m_pop[idx] = s;
            m_BNL.push_back(m_NL[i]); // saving the good neighbors to use them more
            m_changed[idx] = true;
        } else {
            m_changed[idx] = false;
        }
    }

    update_neighborhood();
}

void DE_ABC::replace_unchanged() {

    // modifying unm_changed solutions
    for (size_t i = 0; i < m_pop.size(); i++) {
        if (m_changed[i]) {
            m_changed[i] = false;
            continue;
        }

        size_t idx_to_recalculate = std::numeric_limits<size_t>::max();
        const size_t it = m_params.it();
        for (size_t j = 0; j < it; j++) {
            size_t idx = insertion(m_pop[i]);
            idx_to_recalculate = std::min(idx, idx_to_recalculate);
        }

        core::partial_recalculate_solution(m_instance, m_pop[i], idx_to_recalculate);
    }
}

void DE_ABC::replace_worst_solution(Solution &s) {

    // find the worst solution
    size_t worst = 0;
    for (size_t i = 1; i < m_pop.size(); i++) {
        if (m_pop[i].cost > m_pop[worst].cost) {
            worst = i;
        }
    }

    // check wether the worst in the population is worst than the new generated
    if (m_pop[worst].cost > s.cost) {
        m_pop[worst] = s;
    }
}

size_t DE_ABC::find_best_solution() {
    size_t best = 0;
    for (size_t i = 1; i < m_pop.size(); i++) {
        if (m_pop[i].cost < m_pop[best].cost) {
            best = i;
        }
    }

    return best;
}

void DE_ABC::local_search() {
    size_t idx = 0;

    for (size_t i = 0; i < m_instance.num_jobs(); i++) {
        if (RNG::instance().generate_real_number(0, 1) < m_params.pls()) {
            idx = tournament();
            if (rls(m_pop[idx], m_instance)) {
                m_changed[idx] = true;
            } else {
                m_changed[idx] = false;
            }
            core::recalculate_solution(m_instance, m_pop[idx]);
        } else {
            i--;
        }
    }
}

Solution DE_ABC::solve() {

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

    while (true) {
        Solution s = generate_new_solution();

        replace_worst_solution(s);

        self_adaptative();

        local_search();

        idx = find_best_solution();

        if (!ro.empty() && uptime() >= (ro.back() * mxn) / 1000) {

            std::cout << best_solution.cost << '\n';
            ro.pop_back();
        }

        if (uptime() > m_time_limit) {
            break;
        }

        if (m_pop[idx].cost < best_solution.cost) {
            best_solution = m_pop[idx];
        }

        replace_unchanged();
    }

    m_pop.clear();
    return best_solution;
}
