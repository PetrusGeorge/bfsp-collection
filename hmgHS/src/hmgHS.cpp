#include "hmgHS.h"

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

hmgHS::hmgHS(Instance instance, Parameters params) : m_instance(std::move(instance)), m_params(std::move(params)) {
    if (auto tl = m_params.tl()) {
        this->m_time_limit = *tl;
    } else {
        this->m_time_limit = m_params.ro() * m_instance.num_jobs() * m_instance.num_machines() / 1000;
    }

    m_min = std::vector<double>(m_instance.num_jobs(), std::numeric_limits<double>::max());
    m_max = std::vector<double>(m_instance.num_jobs(), std::numeric_limits<double>::lowest());
}

void hmgHS::check_minmax(Solution &s) {

    for(size_t i = 0; i < m_instance.num_jobs(); i++) {
        if(s.harmony[i] < m_min[i]) {
            m_min[i] = s.harmony[i];
        } 
        if(s.harmony[i] > m_max[i]) {
            m_max[i] = s.harmony[i];
        }
    }

}

void hmgHS::generate_initial_pop() {
    m_pop = std::vector<Solution>(m_params.ms());
    Solution s;
    s.harmony = std::vector<double>(m_instance.num_jobs(), 0.0);
    s.sequence = std::vector<size_t>(m_instance.num_jobs(), 0);

    NEH neh(m_instance);
    const std::vector<size_t> &pts = m_instance.processing_times_sum();
    std::vector<size_t> phi(m_instance.num_jobs());
    
    std::iota(phi.begin(), phi.end(), 0);
    std::sort(phi.begin(), phi.end(), [pts](size_t a, size_t b) { return pts[a] > pts[b]; });
    m_pop[0] = neh.solve(phi);
    core::recalculate_solution(m_instance, m_pop[0]);

    std::iota(phi.begin(), phi.end(), 0);
    std::sort(phi.begin(), phi.end(), [pts](size_t a, size_t b) { return pts[a] < pts[b]; });
    m_pop[1] = neh.solve(phi);
    core::recalculate_solution(m_instance, m_pop[1]);

    for (size_t i = 2; i < m_params.ms(); i++) {

        generate_random_harmony(s);

        check_minmax(s);
        
        harmony_to_permutation(s);

        core::recalculate_solution(m_instance, s);

        revision(s);

        sort_permutation(s);
        
        m_pop[i] = s;
    }

    m_pop[0].harmony = std::vector<double>(m_instance.num_jobs(), 0.0);
    m_pop[1].harmony = std::vector<double>(m_instance.num_jobs(), 0.0);
    
    permutation_to_harmony(m_pop[0]);
    permutation_to_harmony(m_pop[1]);

    sort_permutation(m_pop[0]);
    sort_permutation(m_pop[1]);

    check_minmax(m_pop[0]);
    check_minmax(m_pop[1]);
}

std::vector<size_t> hmgHS::generate_random_sequence() {

    std::vector<size_t> v(m_instance.num_jobs());
    std::iota(v.begin(), v.end(), 0);
    std::shuffle(v.begin(), v.end(), RNG::instance().gen());

    return v;
}

void hmgHS::generate_random_harmony(Solution &sol) {
    size_t n = m_instance.num_jobs();

    for (size_t j = 0; j < n; j++) {
        double r = RNG::instance().generate_real_number(0.0, 1.0);
        sol.harmony[j] = (2 * r) - 1;
    }

}

void hmgHS::permutation_to_harmony(Solution &s) {
    size_t n = m_instance.num_jobs();

    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();

    // getting the minimum and maximum value in the initial harmony memory
    for (size_t i = 0; i < m_params.ms(); i++) {

        auto [s_min, s_max] = std::minmax_element(m_pop[i].harmony.begin(), m_pop[i].harmony.end());
        if (*s_min < min) {
            min = *s_min;
        }

        if (*s_max > max) {
            max = *s_max;
        }
    }

    double normalized_delta = (max - min) / (n - 1);

    // converting permutation using the formula
    for (size_t j = 0; j < n; j++) {
        s.harmony[s.sequence[j]] = max - (normalized_delta * j);
    }
}

void hmgHS::harmony_to_permutation(Solution &s) {
    std::iota(s.sequence.begin(), s.sequence.end(), 0);

    auto sort_criteria = [&s](size_t i, size_t j) { return s.harmony[i] > s.harmony[j]; };

    std::sort(s.sequence.begin(), s.sequence.end(), sort_criteria);

}

std::pair<double, double> hmgHS::get_min_max_of_position(size_t j) {
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();
    for (size_t i = 0; i < m_params.ms(); i++) {

        if (m_pop[i].harmony[j] > max) {
            max = m_pop[i].harmony[j];
        }

        if (m_pop[i].harmony[j] < min) {
            min = m_pop[i].harmony[j];
        }
    }

    return std::make_pair(min, max);
}

void hmgHS::improvise_new_harmony(Solution &sol) {

    size_t n = m_instance.num_jobs();

    for (size_t j = 0; j < n; j++) {

        if (RNG::instance().generate_real_number(0.0, 1.0) < m_params.pcr()) {
            
            if (RNG::instance().generate_real_number(0.0, 1.0) < m_params.par()) {
                sol.harmony[j] = m_pop[0].harmony[j]; // taking the value of the best harmony
            } else {
                size_t alpha = RNG::instance().generate((size_t)0, m_params.ms() - 1);
                sol.harmony[j] = m_pop[alpha].harmony[j];
            }

        } else {

            double min = m_min[j];
            double max = m_max[j];

            double r3 = RNG::instance().generate_real_number(0.0, 1.0);
            sol.harmony[j] = min + (max - min) * r3;

            if(sol.harmony[j] < m_min[j]) {
                m_min[j] = sol.harmony[j];
            } 
            if(sol.harmony[j] > m_max[j]) {
                m_max[j] = sol.harmony[j];
            }
            
        }
    }

}

void hmgHS::revision(Solution &s) {
    size_t n = m_instance.num_jobs();

    std::sort(s.harmony.begin(), s.harmony.end(), [](double a, double b) { return a > b; });

    for (size_t j = 0; j < n - 1; j++) {
        if (s.harmony[j] == s.harmony[j + 1]) {
            if (j != 0) {
                s.harmony[j] += (s.harmony[j - 1] - s.harmony[j]) / n;
            } else {
                s.harmony[j] += 0.01;
            }
        }
    }
}

void hmgHS::sort_permutation(Solution &s) {

    size_t n = m_instance.num_jobs();

    size_t i = 0;

    for (size_t cnt = 0; cnt < n; cnt++) {

        while (i < n && i == s.sequence[i]) {
            i++;
            continue;
        }

        if (i == n) {
            return;
        }

        size_t idx = s.sequence[i];
        std::swap(s.sequence[i], s.sequence[idx]);
        std::swap(s.harmony[i], s.harmony[idx]);
    }

}

void hmgHS::update(Solution &s) {
    size_t i = m_params.ms() - 1;
    while (s.cost < m_pop[i].cost) {
        i--;
        if (i > m_params.ms()) {
            break;
        }
    }

    i++;

    if (i < m_params.ms()) {
        sort_permutation(s);
        m_pop.insert(m_pop.begin() + i, s);
        m_pop.pop_back();
    }
}

Solution hmgHS::solve() {
    size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    std::vector<size_t> ro;
    if (m_params.benchmark()){
        ro = {90, 60, 30};
    }

    uptime();
    Solution best_solution;
    std::vector<size_t> ref(m_instance.num_jobs());
    std::iota(ref.begin(), ref.end(), 0);

    generate_initial_pop();

    std::sort(m_pop.begin(), m_pop.end(), [](Solution &s1, Solution &s2) { return s1.cost < s2.cost; });

    best_solution = m_pop[0];

    Solution new_solution;
    new_solution.harmony = std::vector<double>(m_instance.num_jobs());
    new_solution.sequence = std::vector<size_t>(m_instance.num_jobs());
    while (true) {
        
        improvise_new_harmony(new_solution);

        harmony_to_permutation(new_solution);
        
        revision(new_solution);

        std::shuffle(ref.begin(), ref.end(), RNG::instance().gen());
        rls(new_solution, ref, m_instance);
        core::recalculate_solution(m_instance, new_solution);
        
        update(new_solution);

        if (!ro.empty() && uptime() >= (ro.back()*mxn) / 1000){
            
            std::cout << best_solution.cost << '\n';
            ro.pop_back();
        }

        if (uptime() > m_time_limit) {
            break;
        }

        if (m_pop[0].cost < best_solution.cost) {
            best_solution = m_pop[0];
        }

    }

    m_pop.clear();
    return best_solution;
}
