#include "constructions/MinMax.h"
#include "Core.h"
#include "Parameters.h"
#include <functional>

MinMax::MinMax(Instance& instance, Parameters& params, double alpha) : m_instance(instance), m_params(params), m_alpha(alpha){}

Solution MinMax::solve(){

    std::vector<size_t> sequence;

    std::function<int(size_t, size_t)> p = [this](size_t i, size_t j) {
        return m_instance.p(i, j);  // Call m_instance.p through the lambda
    };
    size_t first_node = 0;

    for (size_t i = 1; i < m_instance.num_jobs(); i++) {

        const size_t first_machine = 0;

        // Choose the minimum processing on the first machine for the first node
        if (p(i, first_machine) < p(first_node, first_machine)) {
            first_node = i;
        }
    }

    size_t last_node = 0;
    if (first_node == last_node) {
        // Avoid first_node being equal to last_node in case 0 is best in both cases
        last_node = 1;
    }
    for (size_t i = 1; i < m_instance.num_jobs(); i++) {
        const size_t last_machine = m_instance.num_machines() - 1;

        // Choose the minimum processing on the last machine for the last node
        if (p(i, last_machine) < p(last_node, last_machine) && i != first_node) {
            last_node = i;
        }
    }

    // Put the remaining nodes in a vector
    std::vector<size_t> cl;
    cl.reserve(m_instance.num_jobs());
    for (size_t i = 0; i < m_instance.num_jobs(); i++) {
        if (i != first_node && i != last_node) {
            cl.push_back(i);
        }
    }

    // Expression number 3 from Roconi paper, https://doi.org/10.1016/S0925-5273(03)00065-3
    auto expression = [this, p](size_t c, size_t last) {
        double lhs_value = 0;
        double rhs_value = 0;

        for (size_t j = 0; j < m_instance.num_machines() - 1; j++) {
            lhs_value += (double)std::abs(p(c, j) - p(last, j + 1));
        }
        lhs_value *= m_alpha;

        for (size_t j = 0; j < m_instance.num_machines(); j++) {
            rhs_value += (double)p(c, j);
        }
        rhs_value *= 1 - m_alpha;

        return rhs_value + lhs_value;
    };

    sequence.push_back(first_node);
    // Choose and insert best value node until there is no one left
    while (!cl.empty()) {
        size_t best_node_index = 0;
        double best_node_value = std::numeric_limits<double>::max();
        const size_t last_inserted = sequence.back();

        for (size_t i = 0; i < cl.size(); i++) {

            const double value = expression(cl[i], last_inserted);
            if (value < best_node_value) {
                best_node_value = value;
                best_node_index = i;
            }
        }

        sequence.push_back(cl[best_node_index]);
        cl.erase(cl.begin() + (long)best_node_index);
    }

    sequence.push_back(last_node);

    Solution s;
    s.sequence = sequence;
    s.departure_times = core::calculate_departure_times(m_instance, sequence);
    s.cost = s.departure_times.back().back();

    return s;
}
