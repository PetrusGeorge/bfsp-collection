#include "IG.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
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

// Taillard data structure for bfsp neh
// TODO: refactor this later
struct TaillardDS{
    std::vector<std::vector<size_t>> e; // Earliest completion time
    std::vector<std::vector<size_t>> q; // Tail duration
    std::vector<std::vector<size_t>> f;

    TaillardDS() = delete;
    TaillardDS(const std::vector<size_t> &sequence, size_t k, const Instance &instance){
        e = std::vector(sequence.size() + 1, std::vector<size_t>(instance.num_machines() + 1, 0));
        q = std::vector(sequence.size() + 1, std::vector<size_t>(instance.num_machines() + 1, 0));


        for(size_t i = 1; i <= sequence.size(); i++){
            for(size_t j = 1; j < instance.num_machines(); j++){
                e[i][j] = std::max(e[i][j-1], e[i-1][j+1]) + instance.p(sequence[i-1], j-1);
            }
            e[i].back() = e[i][instance.num_machines() - 1] + instance.p(sequence[i-1], instance.num_machines() - 1);
        }
        DEBUG << "K: " << k << '\n';
        DEBUG_EXTRA << "E: \n";
        for(auto &v : e){
            for(auto a : v){
                DEBUG << a << ' ';
            }DEBUG << '\n';
        }DEBUG << '\n';

        // Reverse the order of the machine AND jobs, opposed to the reversibility
        // property this also goes through jobs in the reverse order
        // Also q itself is filled in the reverse way
        for(long i = (long)sequence.size()-1; i >= 0; i--){
            for(long j = (long)instance.num_machines()-1; j > 0 ; j--){
                q[i][j] = std::max(q[i][j+1], q[i+1][j-1]) + instance.p(sequence[i], j);
            }
            q[i][0] = q[i][1] + instance.p(sequence[i], 0);
        }
        DEBUG_EXTRA << "Q: \n";
        for(auto &v : q){
            for(auto a : v){
                DEBUG << a << ' ';
            }DEBUG << '\n';
        }DEBUG << '\n';

        // f needs to store all possibilities of insertion so it has sequence.size + 1
        f = std::vector(sequence.size() + 1, std::vector<size_t>(instance.num_machines() + 1, 0));
        for(size_t i = 0; i <= sequence.size(); i++){
            for(size_t j = 1; j < instance.num_machines()+1; j++){
                f[i][j] = std::max(f[i][j-1], e[i][j]) + instance.p(k, j-1);
            }
        }
        DEBUG_EXTRA << "Q: \n";
        for(auto &v : f){
            for(auto a : v){
                DEBUG << a << ' ';
            }DEBUG << '\n';
        }DEBUG << '\n';
    }
    // This functions returns {best_index, makespan} receiveng the tie breaking method
    std::pair<size_t, size_t> get_best_insertion(){
        size_t best_index = 0;
        size_t best_value = std::numeric_limits<size_t>::max();

        DEBUG_EXTRA << "Sums:\n";
        for(size_t i = 0; i < f.size(); i++){
            size_t max_value = 0;
            for(size_t j = 1; j < f[i].size(); j++){
                DEBUG << f[i][j] + q[i][j-1] << ' ';
                max_value = std::max(f[i][j] + q[i][j-1], max_value);
            }DEBUG << '\n';
            if(max_value < best_value){
                best_value = max_value;
                best_index = i;
            }
        }
        DEBUG_EXTRA << "Best index: " << best_index << ' ' << "Best value: " << best_value << '\n';
        return {best_index, best_value};
    }
};

IG::IG(Instance &&instance, Parameters &&params) : m_instance(std::move(instance)), m_params(std::move(params)) {}

std::vector<std::vector<size_t>> IG::calculate_departure_times(const std::vector<size_t> &sequence, bool reverse){
    
    auto departure_times = std::vector(sequence.size(), std::vector<size_t>(m_instance.num_machines()));

    std::function<long(size_t, size_t)> p = get_reversible_matrix(reverse);

    // Calculate first job
    departure_times[0][0] = p(sequence[0], 0);
    for (size_t j = 1; j < m_instance.num_machines(); j++) {
        departure_times[0][j] = departure_times[0][j - 1] + p(sequence[0], j);
    }

    for (size_t i = 1; i < sequence.size(); i++) {
        const size_t node = sequence[i];
        departure_times[i][0] =
            std::max(departure_times[i - 1][0] + p(node, 0), departure_times[i - 1][1]);
        for (size_t j = 1; j < m_instance.num_machines() - 1; j++) {

            const size_t current_finish_time = departure_times[i][j - 1] + p(node, j);

            departure_times[i][j] = std::max(current_finish_time, departure_times[i - 1][j + 1]);
        }
        departure_times[i].back() =
            departure_times[i][m_instance.num_machines() - 2] + p(node, m_instance.num_machines() - 1);
    }

    return departure_times;
}

void IG::recalculate_solution(Solution &s) {

    s.departure_times = calculate_departure_times(s.sequence);
    s.cost = s.departure_times.back().back();
}

std::function<long(size_t, size_t)> IG::get_reversible_matrix(bool reverse) {

    // A c++ hack using lambda functions to call normal matrix view or the reverse one
    std::function<long(size_t, size_t)> p = [this](size_t i, size_t j) { return m_instance.p(i, j); };
    if (reverse) {
        p = [this](size_t i, size_t j) { return m_instance.rp(i, j); };
    }

    return p;
}

std::vector<size_t> IG::min_max(bool reverse) {

    std::vector<size_t> s;

    std::function<long(size_t, size_t)> p = get_reversible_matrix(reverse);

    size_t first_node = 0;

    for (size_t i = 1; i < m_instance.num_jobs(); i++) {

        const size_t first_machine = 0;

        // Choose the minimum processing on the first machine for the first node
        if (p(i, first_machine) < p(first_node, first_machine)) {
            first_node = i;
        }

    }
    size_t last_node = 0;
    if(first_node == 0) {
        last_node = 1;
    }
    for(size_t i = 1; i < m_instance.num_jobs(); i++){
        const size_t last_machine = m_instance.num_machines() - 1;

        // Choose the minimum processing on the last machine for the last node
        if (p(i, last_machine) < p(last_node, last_machine) && i != first_node) {
            last_node = i;
        }
    }
    assert(first_node != last_node);

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
Solution IG::neh(std::vector<size_t> &&phi, bool reverse) {
    // std::function<long(size_t, size_t)> p = get_reversible_matrix(reverse);

    Solution s;
    s.sequence.reserve(m_instance.num_jobs());
    s.sequence = {phi[0], phi[1]};
    phi.erase(phi.begin(), phi.begin()+2);
    DEBUG_EXTRA << "phi size: " << phi.size() << '\n';

    recalculate_solution(s);
    size_t cost_1 = s.cost;

    std::swap(s.sequence[0], s.sequence[1]);
    recalculate_solution(s);
    size_t cost_2 = s.cost;

    if(cost_1 < cost_2){
        std::swap(s.sequence[0], s.sequence[1]);
    }

    while(!phi.empty()){
        DEBUG_EXTRA << "Sequence: ";
        for(auto a : s.sequence){
            DEBUG << a << ' ';
        }DEBUG << '\n';
        TaillardDS tds(s.sequence, phi.front(), m_instance);
        auto [best_index, makespan] = tds.get_best_insertion();
        s.sequence.insert(s.sequence.begin() + (long)best_index, phi.front());
        recalculate_solution(s); // DEBUG
        DEBUG_EXTRA << "Sequence: ";
        for(auto a : s.sequence){
            DEBUG << a << ' ';
        }DEBUG << '\n';
        DEBUG_EXTRA << "Makespan returned and real cost\n";
        DEBUG << makespan << ' ' << s.cost << '\n';
        phi.erase(phi.begin());
    }
    exit(0);

    return s;
}

Solution IG::initial_solution() {
    Solution best;

    DEBUG_EXTRA << "Normal neh\n";
    Solution normal = neh(min_max());
    recalculate_solution(normal);

    DEBUG_EXTRA << "Reverse neh\n";
    Solution reverse = neh(min_max(true), true);
    std::reverse(reverse.sequence.begin(), reverse.sequence.end());
    recalculate_solution(reverse);

    best = normal.cost < reverse.cost ? std::move(normal) : std::move(reverse);

    return best;
}

Solution IG::solve() {

    Solution current = initial_solution();
    m_best = current;

    const size_t time_limit = m_params.ro() * m_instance.num_jobs() * m_instance.num_machines();

    while (uptime() < time_limit) {
        Solution incumbent = local_search(current);

        if (incumbent.cost < m_best.cost) {
            m_best = current = std::move(incumbent);
        }
        // If the solution is worse than the best it's accepted 50% of the times
        else if (RNG::instance().generate(0, 1) == 1) {
            current = std::move(incumbent);
        }

        destroy(current);
        construct(current);
    }

    return std::move(m_best);
}

// TODO:NEDA local search
Solution IG::local_search(Solution s) { return s; }

void IG::destroy(Solution &s) {
    // Random remove d nodes
    for (size_t i = 0; i < m_params.d(); i++) {
        s.sequence.erase(s.sequence.begin() + RNG::instance().generate<long>(0, (long)s.sequence.size() - 1));
    }
}

// TODO: Random insertion
void IG::construct(Solution &s) {}
