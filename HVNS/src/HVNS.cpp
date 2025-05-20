#include "HVNS.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <math.h>

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
    std::vector<size_t> vec_processing_times_sum =
        m_instance.processing_times_sum();

    for (size_t i = 0; i < vec_processing_times_sum.size(); i++) {
        processing_times_sum += vec_processing_times_sum[i];
    }

    m_T_init = static_cast<double>(processing_times_sum) / (m_instance.num_jobs() * m_instance.num_machines() * 5);
    m_T = m_T_init;
    m_T_fin = m_T_init * 0.1;
    m_n_iter = 0;
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

        auto [index, obj] = helper.taillard_best_insertion(s.sequence, job, std::numeric_limits<size_t>::infinity());

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

/*
We aren't sure if this algorithm use the taillard speed up or not
*/
bool HVNS::best_edge_insertion(Solution &s) {

    bool improved = false;
    NEH helper(m_instance);
    size_t best_job;
    size_t best_obj = s.cost;
    size_t best_index = std::numeric_limits<size_t>::infinity();

    for(size_t i = 0; i < m_instance.num_jobs(); i++) {

        const size_t job = s.sequence[i];
        s.sequence.erase(s.sequence.begin() + (long)i, s.sequence.begin() + (long)i+2);

        auto [index, obj] = helper.taillard_best_insertion(s.sequence, job, std::numeric_limits<size_t>::infinity());

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

void HVNS::sa_rls(Solution &s, Solution &best) {

    std::vector<size_t> ref = generate_random_sequence();
    size_t j = 0;
    size_t cnt = 0;
    NEH helper(m_instance);
    size_t curr_makespan = s.cost;
    while (cnt < m_instance.num_jobs()) {
        j = (j + 1) % m_instance.num_jobs();
        const size_t job = ref[j];

        size_t i;
        for (i = 0; i < s.sequence.size(); i++) {
            if (s.sequence[i] == job) {
                s.sequence.erase(s.sequence.begin() + (long)i);
            }
        }

        auto [best_index, best_nei_obj] = helper.taillard_best_insertion(s.sequence, job, i);
        
        double delta = static_cast<double>(best_nei_obj - curr_makespan);
        if (RNG::instance().generate_real_number(0, 1) < 1 / exp(delta / m_T)) {
            s.sequence.insert(s.sequence.begin() + (long)best_index, job);
            cnt = 0;
            std::shuffle(ref.begin(), ref.end(), RNG::instance().gen());
            s.cost = best_nei_obj;
        } else {   
            s.sequence.insert(s.sequence.begin() + (long)i, job);
            cnt++;
        }

        if (best_nei_obj < best.cost) {
            best = s;
        }

        m_n_iter++;
        double beta = (m_T_init - m_T_fin) / (m_n_iter * m_T_init * m_T_fin);
        m_T = m_T / (1 + (beta * m_T));
    }
}

Solution HVNS::solve() {

    size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    std::vector<size_t> ro;
    if (m_params.benchmark()){
        ro = {90, 60, 30};
    }

    Solution best_solution, current;

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
                sa_rls(current, best_solution);
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