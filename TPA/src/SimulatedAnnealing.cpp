#include "SimulatedAnnealing.h"
#include "constructions/NEH.h"

#include "Core.h"
#include "Instance.h"
#include "RNG.h"
#include "Solution.h"
#include <chrono>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace {
size_t uptime() {
    static const auto global_start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - global_start_time);
    return duration.count();
}
} // namespace

SimulatedAnnealing::SimulatedAnnealing(Solution &solution, Instance &instance, Parameters &params)
    : m_solution(solution), m_instance(instance),  m_params(params), m_n_iter(m_params.n_iter()), m_final_temp(m_params.final_temperature()), 
    m_initial_temp(0), m_decay(0) {
    calculate_initial_temp();
    calculate_decay();
}

std::vector<size_t> SimulatedAnnealing::generate_random_sequence() {

    std::vector<size_t> v(m_instance.num_jobs());
    std::iota(v.begin(), v.end(), 0);
    std::shuffle(v.begin(), v.end(), RNG::instance().gen());

    return v;
}

Solution SimulatedAnnealing::solve() {

    size_t time_limit = 0;
    const size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    if (auto tl = m_params.tl()) {
        time_limit = *tl;
    } else {
        time_limit = (m_params.ro() * mxn) / 1000;
    }

    std::vector<size_t> ro;
    if (m_params.benchmark()) {
        time_limit = (100 * mxn) / 1000; 
        ro = {90, 60, 30};
    }

    double current_temp = m_initial_temp;
    size_t reference_cost = m_solution.cost;
    size_t best_cost = m_solution.cost;

    size_t n_jobs = m_instance.num_jobs();

    Solution best_solution = m_solution;
    Solution current_solution = m_solution;

    while (true) {
        auto position = RNG::instance().generate<size_t>(0, n_jobs - 1);
        Solution new_sol = anneal(current_solution, position);

        int delta = (int)new_sol.cost - (int)current_solution.cost;

        if (!ro.empty() && uptime() >= (ro.back() * mxn) / 1000) {

            std::cout << best_cost << '\n';
            ro.pop_back();
        }
        //  Program should not accept any solution if the time is out
        if (uptime() > time_limit) {
            break;
        }
        
        if (delta <= 0) {
            current_solution.sequence = new_sol.sequence;
            core::recalculate_solution(m_instance, current_solution);
            reference_cost = new_sol.cost;

            if (reference_cost < best_cost) {
                best_solution = new_sol;
                best_cost = reference_cost;
            }
        } else {
            double acceptance_probability = std::exp(-delta / current_temp);
            auto random = RNG::instance().generate_real_number(0, 1);

            if (acceptance_probability > random) {
                current_solution.sequence = new_sol.sequence;
                core::recalculate_solution(m_instance, current_solution);
                reference_cost = new_sol.cost;
            }
        }

        current_temp = current_temp / (1 + (m_decay * current_temp));
    }

    return best_solution;
}

void SimulatedAnnealing::calculate_initial_temp() {
    size_t sum = 0;
    for (size_t i = 0; i < m_instance.num_jobs(); i++) {
        for (size_t j = 0; j < m_instance.num_machines(); j++) {
            sum += m_instance.p(i, j);
        }
    }

    m_initial_temp = (double)sum / (double)(5 * m_instance.num_jobs() * m_instance.num_machines());
}

void SimulatedAnnealing::calculate_decay() {
    double delta_temp = m_initial_temp - m_final_temp;

    m_decay = delta_temp / ((m_n_iter - 1) * m_initial_temp * m_final_temp);
}

Solution SimulatedAnnealing::anneal(Solution &current_solution, size_t position) {
    NEH helper(m_instance);

    Solution new_sol = current_solution;
    size_t job = current_solution.sequence[position];
    new_sol.sequence.erase(new_sol.sequence.begin() + position);

    auto [best_index, makespan] = helper.mtaillard_best_insertion(new_sol.sequence, job, position);

    new_sol.sequence.insert(new_sol.sequence.begin() + best_index, job);
    new_sol.cost = makespan;

    return new_sol;
}
