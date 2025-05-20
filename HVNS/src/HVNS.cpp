#include "HVNS.h"

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

HVNS::HVNS(Instance instance, Parameters params) : m_instance(std::move(instance)), m_params(std::move(params)) {
    if (auto tl = m_params.time_limit()) {
        m_time_limit = *tl;
    } else {
        this->m_time_limit = m_params.ro() * m_instance.num_jobs() * m_instance.num_machines() / 1000;
    }
}

Solution HVNS::generate_first_solution() {

    std::vector<size_t> sorted_sequece = core::stpt_sort(m_instance);

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

    for (size_t i = 0; i < s1.sequence.size(); i++) {

        if (s1.sequence[i] != s2.sequence[i]) {
            return false;
        }
    }

    return true;
}


bool HVNS::best_insertion(Solution &s) {

    bool improved = false;
    NEH helper(m_instance);
    size_t best_job;
    size_t best_obj = s.cost;
    size_t best_index = std::numeric_limits<size_t>::infinity();

    for(size_t i = 0; i < m_instance.num_jobs(); i++) {

        const size_t job = s.sequence[i];
        s.sequence.erase(s.sequence.begin() + (long)i);

        auto [index, obj] = helper.taillard_best_insertion(s.sequence, job);

        s.sequence.insert(s.sequence.begin() + (long)i, job);

        if (obj < best_obj) {
            best_obj = obj;
            best_index = index;
            best_job = i;
            improved = true;
        }
    }

    if (!(best_index < std::numeric_limits<size_t>::infinity())) {
        return improved;
    }

    if (best_job < best_index) {
        std::rotate(s.sequence.begin()+best_job, s.sequence.begin()+best_job+1, s.sequence.begin()+best_index);
    } else {
        std::rotate(s.sequence.begin()+best_index, s.sequence.begin()+best_job, s.sequence.begin()+best_job+1);
    }

    return improved;
}

std::pair<size_t, size_t> HVNS::taillard_best_edge_insertion(const std::vector<size_t> &sequence, std::pair<size_t, size_t> &jobs) {
    std::vector<std::vector<size_t>> m_e = core::calculate_departure_times(m_instance, sequence);

    std::vector<std::vector<size_t>> m_q = core::calculate_tail(m_instance, sequence);
    // Make it easier to implement find_best_insertion
    // without an out of bound access
    m_q.emplace_back(m_instance.num_machines(), 0);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // std::cout << "Departure Times" << std::endl;
    // for(size_t i = 0; i < m_e.size(); i++) {
    //     for(size_t j = 0; j < m_e[i].size(); j++) {
    //         std::cout << m_e[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;

    // std::cout << "Tail" << std::endl;
    // for(size_t i = 0; i < m_q.size(); i++) {
    //     for(size_t j = 0; j < m_q[i].size(); j++) {
    //         std::cout << m_q[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    auto p = [this](size_t i, size_t j) { return m_instance.p(i, j); };

    // f needs to store all possibilities of insertion so it has sequence.size + 1
    std::vector<std::vector<size_t>> m_f = std::vector(sequence.size() + 1, std::vector<size_t>(m_instance.num_machines()));

    // Evaluate best insertion
    size_t max_value = 0;
    auto set_f_and_max = [&m_q, &m_f, &max_value](size_t i, size_t j, size_t value) {
        m_f[i][j] = value;
        max_value = std::max(value + m_q[i][j], max_value);
    };
    
    set_f_and_max(0, 0, p(jobs.first, 0));
    for (size_t j = 1; j < m_instance.num_machines(); j++) {
        set_f_and_max(0, j, m_f[0][j - 1] + p(jobs.first, j));
    }

    set_f_and_max(0, 0, m_f[0][0] + p(jobs.second, 0));
    for (size_t j = 1; j < m_instance.num_machines()-1; j++) {
        size_t value = std::max(m_f[0][j-1] + p(jobs.second, j), m_f[0][j+1]);
        set_f_and_max(0, j, value);
    }
    size_t value = m_f[0][m_instance.num_machines() - 2] + p(jobs.second, m_instance.num_machines() - 1);
    set_f_and_max(0, m_instance.num_machines() - 1, value);

    size_t best_index = 0;
    size_t best_value = max_value;

    for (size_t i = 1; i <= sequence.size(); i++) {
        max_value = 0;
        value = std::max(m_e[i - 1][0] + p(jobs.first, 0), m_e[i - 1][1]);
        set_f_and_max(i, 0, value);

        for (size_t j = 1; j < m_instance.num_machines() - 1; j++) {
            value = std::max(m_f[i][j - 1] + p(jobs.first, j), m_e[i - 1][j + 1]);
            set_f_and_max(i, j, value);
        }
        value = m_f[i][m_instance.num_machines() - 2] + p(jobs.first, m_instance.num_machines() - 1);
        set_f_and_max(i, m_instance.num_machines() - 1, value);

///////////////////////////////////////////////////////////////////////////////////////////////////////////

        max_value = 0;
        value = std::max(m_f[i][0] + p(jobs.second, 0), m_f[i][1]);
        set_f_and_max(i, 0, value);

        for (size_t j = 1; j < m_instance.num_machines() - 1; j++) {
            value = std::max(m_f[i][j - 1] + p(jobs.second, j), m_f[i][j + 1]);
            set_f_and_max(i, j, value);
        }
        value = m_f[i][m_instance.num_machines() - 2] + p(jobs.second, m_instance.num_machines() - 1);
        set_f_and_max(i, m_instance.num_machines() - 1, value);

///////////////////////////////////////////////////////////////////////////////////////////////////////////

        if (max_value < best_value) {
            best_value = max_value;
            best_index = i;
        }
    }


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // std::cout << "M_F" << std::endl;
    // for(size_t i = 0; i < m_f.size(); i++) {
    //     for(size_t j = 0; j < m_f[i].size(); j++) {
    //         std::cout << m_f[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << std::endl;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    return {best_index, best_value};
}

bool HVNS::best_edge_insertion(Solution &s) {

    bool improved = false;
    NEH helper(m_instance);
    size_t best_job;
    size_t best_obj = s.cost;
    size_t best_index = std::numeric_limits<size_t>::infinity();

    for(size_t i = 0; i < m_instance.num_jobs(); i++) {

        const size_t job = s.sequence[i];
        s.sequence.erase(s.sequence.begin() + (long)i, s.sequence.begin() + (long)i+2);

        auto [index, obj] = helper.taillard_best_insertion(s.sequence, job);

        s.sequence.insert(s.sequence.begin() + (long)i, job);

        if (obj < best_obj) {
            best_obj = obj;
            best_index = index;
            best_job = i;
            improved = true;
        }
    }

    if (!(best_index < std::numeric_limits<size_t>::infinity())) {
        return improved;
    }

    if (best_job < best_index) {
        std::rotate(s.sequence.begin()+best_job, s.sequence.begin()+best_job+1, s.sequence.begin()+best_index);
    } else {
        std::rotate(s.sequence.begin()+best_index, s.sequence.begin()+best_job, s.sequence.begin()+best_job+1);
    }

    return improved;
}

bool HVNS::best_swap(Solution &s) {

    Solution copy = s;
    bool improved = false;
    size_t best_i;
    size_t best_j;
    size_t best_obj = s.cost;

    for (size_t i = 0; i < m_instance.num_jobs(); i++) {

        for (size_t j = i+1; j < m_instance.num_jobs(); j++) {

            std::swap(copy.sequence[i], copy.sequence[j]);

            core::partial_recalculate_solution(m_instance, copy, i);

            std::swap(copy.sequence[i], copy.sequence[j]);

            if (copy.cost < best_obj) {
                best_obj = copy.cost;
                best_i = i;
                best_j = j;
                improved = true;
            }
        }
        
        copy.departure_times[i] = s.departure_times[i];

    }

    if (!improved) {
        return improved;
    }

    std::swap(s.sequence[best_i], s.sequence[best_j]);

    core::partial_recalculate_solution(m_instance, s, best_i);

    return improved;
}

void HVNS::shaking(Solution &s, size_t k) {

    Solution copy = s;
    switch (k) {
        case 1:
            best_insertion(copy);
            break;
        case 2:
            best_swap(copy);
            break;
        case 3:
            best_edge_insertion(copy);
            break;
    }

    if (copy.cost < s.cost) {
        s = copy;
    } 

}

Solution HVNS::solve() {

    size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    std::vector<size_t> ro;
    if (m_params.benchmark()){
        ro = {90, 60, 30};
    }

    Solution best_solution, current;
    best_solution.sequence = generate_random_sequence();
    core::recalculate_solution(m_instance, best_solution);
    
    for(size_t i = 0; i < best_solution.sequence.size(); i++) {std::cout << best_solution.sequence[i] << " ";}
    std::cout << std::endl << "makespan: " << best_solution.cost << std::endl;

    std::pair<size_t, size_t> jobs = std::make_pair(best_solution.sequence[0], best_solution.sequence[1]);

    best_solution.sequence.erase(best_solution.sequence.begin(), best_solution.sequence.begin()+2);
    auto [idx, makespan] = taillard_best_edge_insertion(best_solution.sequence, jobs);

    best_solution.sequence.insert(best_solution.sequence.begin()+idx, jobs.first);
    best_solution.sequence.insert(best_solution.sequence.begin()+idx+1, jobs.second);
    
    for(size_t i = 0; i < best_solution.sequence.size(); i++) {std::cout << best_solution.sequence[i] << " ";}
    std::cout << std::endl << "makespan: " << makespan << std::endl;

    core::recalculate_solution(m_instance, best_solution);

    for(size_t i = 0; i < best_solution.sequence.size(); i++) {std::cout << best_solution.sequence[i] << " ";}
    std::cout << std::endl << "makespan: " << best_solution.cost << "\nidx: " << idx << std::endl;

    getchar();

    size_t previous_best_obj = best_solution.cost;

    while (true) {
        size_t k = 1;

        while (k < KMAX) {
            shaking(current, k);
            
            if (current.cost < best_solution.cost) {
                best_solution = current;
            }

            Solution temp;
            do {
                // LC1
                temp = current;
                // LC2
            } while(!equal_solution(current, temp));

            if (best_solution.cost < previous_best_obj) {
                k = 1;
                previous_best_obj = best_solution.cost;
            } else {
                k++;
            }
        }
        if (!ro.empty() && uptime() >= (ro.back()*mxn) / 1000){
                
            std::cout << best_solution.cost << '\n';
            ro.pop_back();
        }
        
        if (uptime() > m_time_limit) {
            break;
        }
    }

    return best_solution;
}