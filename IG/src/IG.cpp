#include "IG.h"

#include <chrono>

#include "Instance.h"
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

IG::IG(Instance &&instance, Parameters &&params) : m_instance(std::move(instance)), m_params(std::move(params)) {}

// TODO: Solution generation
Solution IG::initial_solution() { return {}; }

Solution IG::solve() {

    Solution current = initial_solution();
    m_best = current;

    const size_t time_limit = m_params.ro() * m_instance.num_jobs() * m_instance.num_machines();

    while (uptime() < time_limit) {
        const Solution incumbent = local_search(current);

        if (incumbent.cost < m_best.cost) {
            m_best = current = incumbent;
        }
        // If the solution is worse than the best it's accepted 50% of the times
        else if (RNG::instance().generate(0, 1) == 1) {
            current = incumbent;
        }

        destroy(current);
        construct(current);
    }

    return std::move(m_best);
}

// TODO:NEDA local search
Solution IG::local_search(Solution s) { return s; }

// TODO: Random destroy
void IG::destroy(Solution &s) {}

// TODO: Random insertion
void IG::construct(Solution &s) {}
