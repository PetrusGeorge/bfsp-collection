#include "DE_ABC.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>

namespace {
size_t uptime() {
    static const auto global_start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - global_start_time);
    return duration.count();
}
} 

DE_ABC::DE_ABC(Instance instance, Parameters params) 
: m_instance(std::move(instance)), m_params(std::move(params)) {
    this->m_time_limit = m_params.ro() * m_instance.num_jobs() * m_instance.num_machines();

    // initializing the neighborhood list
    for(size_t i = 0; i < m_params.ps(); i++) {
        NL.push_back(RNG::instance().generate((size_t) 0, (size_t) 3));
    }
    
    changed = std::vector<bool>(m_params.ps(), false);
}

bool DE_ABC::new_in_population(std::vector<size_t> &sequence) {

    // check every solution in population
    for(size_t i = 0; i < m_pop.size(); i++) {

        bool already_exist = true;
        for(size_t j = 0; j < m_pop[i].sequence.size(); j++) {

            if(sequence[j] != m_pop[i].sequence[j]) {
                already_exist = false;
                break;
            }
        }

        if(already_exist) {
            return false;
        }
    }

    return true;
}

void DE_ABC::generate_initial_pop() {

    MinMax mm = MinMax(m_instance, m_params.theta());
    NEH neh = NEH(m_instance);
    Solution first_solution = neh.solve(mm.solve().sequence); // MME heuristic for the first solution
    core::recalculate_solution(m_instance, first_solution);

    m_pop.push_back(first_solution);

    // generating other random solutions
    for (size_t i = 1; i < m_params.ps(); i++) {

        std::vector<size_t> new_seq = generate_random_sequence();
        
        if(!new_in_population(new_seq)) {
            i--;
            continue;
        } 

        Solution s;
        s.sequence = new_seq;
        m_pop.push_back(s);
        core::recalculate_solution(m_instance, m_pop[i]);
    }

}

std::vector<size_t> DE_ABC::generate_random_sequence() {

    std::vector<size_t> v(m_instance.num_jobs());
    std::iota(v.begin(), v.end(), 0);
    std::shuffle(v.begin(), v.end(), RNG::instance().gen());

    return v;
}

size_t DE_ABC::selection() {

    size_t i = RNG::instance().generate((size_t) 0, m_pop.size() - 1);
    size_t j = i;

    while (i == j) {
        j = RNG::instance().generate((size_t) 0, m_pop.size() - 1);
    }

    if (m_pop[i].cost > m_pop[j].cost) {
        return j;
    } else {
        return i;
    }
}

std::vector<size_t> DE_ABC::mutation() {
    size_t n = m_instance.num_jobs();

    // taking three random solutions
    size_t ind_1 = RNG::instance().generate((size_t) 0, m_pop.size()-1);
    size_t ind_2 = ind_1;
    size_t ind_3 = ind_1;

    while(ind_2 == ind_1) {
        ind_2 = RNG::instance().generate((size_t) 0, m_pop.size()-1);
    }

    while(ind_3 == ind_1 || ind_3 == ind_2) {
        ind_3 = RNG::instance().generate((size_t) 0, m_pop.size()-1);
    }

    std::vector<size_t> new_pi(n);
    for(size_t i = 0; i < n; i++) {

        // use the formula given in the article to generate a(n possibly invalid) new solution 
        if(RNG::instance().generate_real_number(0, 1) < m_params.pmu()) {
            new_pi[i] = (m_pop[ind_1].sequence[i] + n + 1 + m_pop[ind_2].sequence[i] - m_pop[ind_3].sequence[i]) % n;
        } else {
            new_pi[i] = (m_pop[ind_1].sequence[i] + n + 1) % n;
        }
    }

    return new_pi;
}

Solution DE_ABC::crossover(std::vector<size_t> &pi) {
    size_t n = m_instance.num_jobs();
    std::vector<size_t> pi_temp;

    // putting some unique jobs into pi_temp
    for(size_t i = 0; i < n; i++) {
        if(RNG::instance().generate_real_number(0, 1) < m_params.pc()) {
            continue;
        } 

        bool out_pi_temp = true;
        for(size_t j = 0; j < pi_temp.size(); j++) {
            if(pi[i] == pi_temp[j]) {
                out_pi_temp = false;
                break;
            }
        }

        if(out_pi_temp) {
            pi_temp.push_back(pi[i]);
        }
    }

    // getting any solution from population as a reference
    std::vector<size_t> ref = m_pop[ RNG::instance().generate((size_t) 0, m_pop.size()-1) ].sequence;
    
    std::vector<size_t> deleted;
    size_t k = 1;
    
    // finding the position of pi_temp jobs jobs within ref
    for(size_t i = ref.size()-1; i < ref.size(); i--) {
    
        for(size_t j = pi_temp.size()-k; j < pi_temp.size(); j--) {
            if(ref[i] == pi_temp[j]) {
                deleted.push_back(i);
                std::swap(pi_temp[j], pi_temp[pi_temp.size()-k]);
                k++;
                break;
            }
        }

        if(pi_temp.size() < k) {
            break;
        }
    }

    // deleting ref jobs are in pi_temp
    for(size_t i = 0; i < deleted.size(); i++) {
        ref.erase(ref.begin()+deleted[i]);
    }

    // neh second step in ref to make ref have all the jobs missing in the best position 
    Solution s;
    s.sequence.swap(ref);

    NEH neh = NEH(m_instance);
    neh.second_step(pi_temp, s);

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
    BNL.resize(BNL.size() * 0.7);
    NL.swap(BNL);
    BNL.clear();
    
    for(size_t i = NL.size(); i < m_params.ps(); i++) {
        NL.push_back(RNG::instance().generate((size_t) 0, (size_t) 3));
    }
}

void DE_ABC::swap(Solution &s) {
    size_t n = m_instance.num_jobs();
    size_t idx_1 = RNG::instance().generate((size_t) 0, n-3);
    size_t idx_2 = idx_1;

    while(idx_1 == idx_2) {
        idx_2 = RNG::instance().generate((size_t) 0, n-1);
    }

    if(idx_1 > idx_2) {
        std::swap(idx_1, idx_2);
    }

    std::swap(s.sequence[idx_1], s.sequence[idx_2]);

    // core::recalculate_solution_from_index(m_instance, s, idx_1);

}

void DE_ABC::insertion(Solution &s) {
    size_t n = m_instance.num_jobs();
    size_t idx_1 = RNG::instance().generate((size_t) 0, n-3);
    size_t idx_2 = idx_1;

    while(idx_1 == idx_2) {
        idx_2 = RNG::instance().generate((size_t) 0, n-1);
    }

    if(idx_1 > idx_2) {
        std::swap(idx_1, idx_2);
    }

    std::rotate(s.sequence.begin()+idx_1+1, s.sequence.begin()+idx_2, s.sequence.begin()+idx_2+1);

    // core::recalculate_solution_from_index(m_instance, s, idx_1+1);
    
}

void DE_ABC::self_adaptative() {
    size_t idx;

    for(size_t i = 0; i < m_params.ps(); i++) {
        idx = selection(); 

        Solution s = m_pop[idx];
        switch (NL[i]) {
            case 0:
                insertion(s);
                break;
            case 1:
                swap(s);
                break;
            case 2:
                insertion(s);
                insertion(s);
                break;
            case 3:
                swap(s);
                swap(s);        
        }

        core::recalculate_solution(m_instance, s);

        if(s.cost < m_pop[idx].cost) {
            m_pop[idx] = s;
            BNL.push_back(NL[i]); // saving the good neighbors to use them more
            changed[idx] = true;
        } 
    }

    update_neighborhood();

}

void DE_ABC::updating_unchanged() {
    size_t n = m_instance.num_jobs();

    // modifying unchanged solutions
    for(size_t i = 0; i < m_pop.size(); i++) {
        if(changed[i]) {
            changed[i] = false;
            continue;
        }

        // i don't know how many insertions i have to do
        for(size_t j = 0; j < n/4; j++) {
            insertion(m_pop[i]);
        }
    }

}

void DE_ABC::replace_worst_solution(Solution &s) {
    
    // finding the best place to put s so that the population is sorted in non-decreasing order of makespan
    size_t i = m_pop.size()-1;
    while(i < m_pop.size() && s.cost < m_pop[i].cost) {
        i--;
    }

    
    if(i == m_pop.size()-1) {
        return;
    } else if(i > m_pop.size()) {
        m_pop.insert(m_pop.begin(), s);
    } else {
        m_pop.insert(m_pop.begin()+i+1, s);
    }

    m_pop.pop_back();
}

Solution DE_ABC::solve() {

    Solution best_solution;
    std::vector<size_t> ref;

    generate_initial_pop();

    auto sort_criteria = [](Solution &p1, Solution &p2) { return p1.cost < p2.cost; };
    std::sort(m_pop.begin(), m_pop.end(), sort_criteria);

    best_solution = m_pop[0];

    while (true) {
        Solution s = generate_new_solution();

        self_adaptative();

        if(RNG::instance().generate_real_number(0, 1) < m_params.pls()) {
            ref = generate_random_sequence();
            rls(s, ref, m_instance);
        }

        replace_worst_solution(s);

        updating_unchanged();

        if (m_pop[0].cost < best_solution.cost) {
            best_solution = m_pop[0];
        }

        if (uptime() > m_time_limit) {
            break;
        }

    }

    m_pop.clear();
    return best_solution;
}
