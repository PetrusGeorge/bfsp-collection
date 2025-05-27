#include "SimulatedAnnealing.h"
#include "constructions/NEH.h"

#include "Core.h"
#include "Instance.h"
#include "RNG.h"
#include "Solution.h"
#include <cassert>
#include <cmath>
#include <cstddef>

SimulatedAnnealing::SimulatedAnnealing(Solution &solution, Instance &instance, double final_temp, int n_iter)
    : m_solution(solution), m_instance(instance), m_n_iter(n_iter), m_final_temp(final_temp), m_initial_temp(0),
      m_decay(0) {
    calculate_initial_temp();
    calculate_decay();
}

Solution SimulatedAnnealing::solve() {

    double current_temp = m_initial_temp;
    size_t reference_cost = m_solution.cost;
    size_t best_cost = m_solution.cost;

    size_t n_jobs = m_instance.num_jobs();

    Solution best_solution = m_solution;
    Solution current_solution = m_solution;

    while (current_temp > m_final_temp) {
        auto position = RNG::instance().generate<size_t>(0, n_jobs - 1);
        Solution new_sol = anneal(current_solution, position);
        core::recalculate_solution(m_instance, new_sol);

        int delta = (int)new_sol.cost - (int)current_solution.cost;

        if (delta <= 0) {
            current_solution = new_sol;
            reference_cost = new_sol.cost;

            if (reference_cost < best_cost) {
                best_solution = new_sol;
                best_cost = reference_cost;
            }
        } else {
            double acceptance_probability = std::exp(-delta / current_temp);
            auto random = RNG::instance().generate_real_number(0, 1);

            if (acceptance_probability > random) {
                current_solution = new_sol;
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

    auto [best_index, makespan] = helper.taillard_best_insertion(new_sol.sequence, job);

    new_sol.sequence.insert(new_sol.sequence.begin() + best_index, job);
    new_sol.cost = makespan;

    return new_sol;
}
