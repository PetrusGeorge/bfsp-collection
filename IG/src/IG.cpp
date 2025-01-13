#include "IG.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <ostream>
#include <vector>

#include "Instance.h"
#include "Log.h"
#include "RNG.h"
#include "Solution.h"

namespace {
size_t uptime() {
    static const auto global_start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - global_start_time);
    return duration.count();
}
} // namespace

IG::IG(Instance instance, Parameters params) : m_instance(std::move(instance)), m_params(std::move(params)) {}

IG::TaillardDS IG::get_taillard(const std::vector<size_t> &sequence, size_t k, bool jobs_reversed) {
    TaillardDS tds;
    tds.e = calculate_departure_times(sequence, jobs_reversed);

    tds.q = calculate_tail(sequence, jobs_reversed);
    tds.q.emplace_back(m_instance.num_machines(),
                       0); // Make it easier to implement find_best_insertion without an out of bound access

    auto p = get_reversible_matrix(jobs_reversed);

    // f needs to store all possibilities of insertion so it has sequence.size + 1
    tds.f = std::vector(sequence.size() + 1, std::vector<size_t>(m_instance.num_machines()));
    tds.f[0][0] = p(k, 0);
    for (size_t j = 1; j < m_instance.num_machines(); j++) {
        tds.f[0][j] = tds.f[0][j - 1] + p(k, j);
    }
    for (size_t i = 1; i <= sequence.size(); i++) {
        tds.f[i][0] = std::max(tds.e[i - 1][0] + p(k, 0), tds.e[i - 1][1]);
        for (size_t j = 1; j < m_instance.num_machines() - 1; j++) {
            tds.f[i][j] = std::max(tds.f[i][j - 1] + p(k, j), tds.e[i - 1][j + 1]);
        }
        tds.f[i].back() = tds.f[i][m_instance.num_machines() - 2] + p(k, m_instance.num_machines() - 1);
    }

    return tds;
}

std::pair<size_t, size_t> IG::get_best_insertion(const TaillardDS &tds) {
    size_t best_index = 0;
    size_t best_value = std::numeric_limits<size_t>::max();

    for (size_t i = 0; i < tds.f.size(); i++) {
        size_t max_value = 0;
        for (size_t j = 0; j < tds.f[i].size(); j++) {
            const size_t sum = tds.f[i][j] + tds.q[i][j];
            max_value = std::max(sum, max_value);
        }

        if (max_value < best_value) {
            best_value = max_value;
            best_index = i;
        }
    }
    return {best_index, best_value};
}

std::vector<std::vector<size_t>> IG::calculate_departure_times(const std::vector<size_t> &sequence,
                                                               bool jobs_reversed) {

    auto departure_times = std::vector(sequence.size(), std::vector<size_t>(m_instance.num_machines()));

    const std::function<long(size_t, size_t)> p = get_reversible_matrix(jobs_reversed);

    // Calculate first job
    departure_times[0][0] = p(sequence[0], 0);
    for (size_t j = 1; j < m_instance.num_machines(); j++) {
        departure_times[0][j] = departure_times[0][j - 1] + p(sequence[0], j);
    }

    for (size_t i = 1; i < sequence.size(); i++) {
        const size_t node = sequence[i];
        departure_times[i][0] = std::max(departure_times[i - 1][0] + p(node, 0), departure_times[i - 1][1]);
        for (size_t j = 1; j < m_instance.num_machines() - 1; j++) {

            const size_t current_finish_time = departure_times[i][j - 1] + p(node, j);

            departure_times[i][j] = std::max(current_finish_time, departure_times[i - 1][j + 1]);
        }
        departure_times[i].back() =
            departure_times[i][m_instance.num_machines() - 2] + p(node, m_instance.num_machines() - 1);
    }

    return departure_times;
}

std::vector<std::vector<size_t>> IG::calculate_tail(const std::vector<size_t> &sequence, bool jobs_reversed) {

    auto tail = std::vector(sequence.size(), std::vector<size_t>(m_instance.num_machines()));

    const std::function<long(size_t, size_t)> p = get_reversible_matrix(jobs_reversed);

    // Calculate first job
    tail.back().back() = p(sequence.back(), m_instance.num_machines() - 1);
    for (long j = (long)m_instance.num_machines() - 2; j >= 0; j--) {
        tail.back()[j] = tail.back()[j + 1] + p(sequence.back(), j);
    }

    for (long i = ((long)sequence.size()) - 2; i >= 0; i--) {
        const size_t node = sequence[i];
        tail[i].back() = std::max(tail[i + 1].back() + p(node, m_instance.num_machines() - 1),
                                  tail[i + 1][m_instance.num_machines() - 2]);
        for (long j = (long)m_instance.num_machines() - 2; j >= 1; j--) {

            const size_t current_finish_time = tail[i][j + 1] + p(node, j);

            tail[i][j] = std::max(current_finish_time, tail[i + 1][j - 1]);
        }
        tail[i][0] = tail[i][1] + p(node, 0);
    }

    return tail;
}
void IG::recalculate_solution(Solution &s) {

    s.departure_times = calculate_departure_times(s.sequence);
    s.cost = s.departure_times.back().back();
}

std::function<long(size_t, size_t)> IG::get_reversible_matrix(bool jobs_reversed) {

    // A c++ hack using lambda functions to call normal matrix view or the jobs_reversed one
    std::function<long(size_t, size_t)> p = [this](size_t i, size_t j) { return m_instance.p(i, j); };
    if (jobs_reversed) {
        p = [this](size_t i, size_t j) { return m_instance.rp(i, j); };
    }

    return p;
}

std::vector<size_t> IG::min_max(bool jobs_reversed) {

    std::vector<size_t> s;

    const std::function<long(size_t, size_t)> p = get_reversible_matrix(jobs_reversed);

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
        lhs_value *= m_params.alpha();

        for (size_t j = 0; j < m_instance.num_machines(); j++) {
            rhs_value += (double)p(c, j);
        }
        rhs_value *= 1 - m_params.alpha();

        return rhs_value + lhs_value;
    };

    s.push_back(first_node);
    // Choose and insert best value node until there is no one left
    while (!cl.empty()) {
        size_t best_node_index = 0;
        double best_node_value = std::numeric_limits<double>::max();
        const size_t last_inserted = s.back();

        for (size_t i = 0; i < cl.size(); i++) {

            const double value = expression(cl[i], last_inserted);
            if (value < best_node_value) {
                best_node_value = value;
                best_node_index = i;
            }
        }

        s.push_back(cl[best_node_index]);
        cl.erase(cl.begin() + (long)best_node_index);
    }

    s.push_back(last_node);
    return s;
}

void IG::neh_second_step(std::vector<size_t> phi, Solution &s, bool jobs_reversed) {
    while (!phi.empty()) {
        const TaillardDS tds = get_taillard(s.sequence, phi.front(), jobs_reversed);
        auto [best_index, makespan] = get_best_insertion(tds);

        s.sequence.insert(s.sequence.begin() + (long)best_index, phi.front());
        s.cost = makespan;

        phi.erase(phi.begin());
    }
}

Solution IG::neh(std::vector<size_t> phi, bool jobs_reversed) {
    Solution s;
    s.sequence.reserve(m_instance.num_jobs());
    s.sequence = {phi[0]};
    phi.erase(phi.begin());

    neh_second_step(std::move(phi), s, jobs_reversed);

    return s;
}

Solution IG::initial_solution() {
    Solution best;

    Solution normal = neh(min_max());
    recalculate_solution(normal);

    Solution jobs_reversed = neh(min_max(true), true);
    std::reverse(jobs_reversed.sequence.begin(), jobs_reversed.sequence.end());
    recalculate_solution(jobs_reversed);

    best = normal.cost < jobs_reversed.cost ? std::move(normal) : std::move(jobs_reversed);

    return best;
}

Solution IG::solve() {

    VERBOSE(m_params.verbose()) << "Initial solution started\n";
    Solution current = initial_solution();
    Solution best = current;
    VERBOSE(m_params.verbose()) << "Initial solution finished, solution obtained:\n";
    VERBOSE(m_params.verbose()) << current;

    const size_t time_limit = m_params.ro() * m_instance.num_jobs() * m_instance.num_machines();
    std::cout << "Time limit: " << time_limit << "s\n";
    size_t timer_counter = 1;

    while (true) {
        Solution incumbent = local_search(current);

        //  Program should not accept any solution if the time is out
        if (uptime() > time_limit) {
            break;
        }
        if (incumbent.cost < best.cost) {
            VERBOSE(m_params.verbose()) << "Local search has found a new best\n";
            VERBOSE(m_params.verbose()) << incumbent;
            best = current = std::move(incumbent);
        }
        // If the solution is worse than the best it's accepted 50% of the times
        else if (RNG::instance().generate(0, 1) == 1) {
            current = std::move(incumbent);
        }

        std::vector<size_t> removed = destroy(current);
        neh_second_step(std::move(removed), current); // Construct phase

        if (uptime() > time_limit) {
            break;
        }
        if (current.cost < best.cost) {
            VERBOSE(m_params.verbose()) << "Destroy and construct has found a new best";
            VERBOSE(m_params.verbose()) << current;
            best = current;
        }

        // Get progress of program
        if (uptime() > timer_counter * (time_limit / 20)) {
            std::cout << timer_counter * (time_limit / 20) << " Seconds out of " << time_limit << '\n';
            timer_counter++;
        }
    }

    return best;
}

bool IG::swap_first_improvement(Solution &s) {
    Solution copy = s;

    for (size_t i = 0; i < s.sequence.size() - 1; i++) {
        for (size_t j = i + 1; j < s.sequence.size(); j++) {
            // Apply move
            std::swap(copy.sequence[i], copy.sequence[j]);
            recalculate_solution(copy);

            if (copy.cost < s.cost) {
                s = std::move(copy);
                return true;
            }

            // Undo move
            std::swap(copy.sequence[i], copy.sequence[j]);
        }
    }
    return false;
}

Solution IG::local_search(Solution s) {
    while (true) {
        if (!swap_first_improvement(s)) {
            break;
        }
    }
    return s;
}

std::vector<size_t> IG::destroy(Solution &s) {

    // This mostly avoids crashes on really small toy instances
    const size_t destroy_size = std::min(s.sequence.size() - 1, m_params.d());

    std::vector<size_t> removed;
    removed.reserve(destroy_size);
    // Random remove d nodes
    for (size_t i = 0; i < destroy_size; i++) {
        const long chose = RNG::instance().generate<long>(0, ((long)s.sequence.size()) - 1);
        removed.push_back(s.sequence[chose]);
        s.sequence.erase(s.sequence.begin() + chose);
    }

    return removed;
}
