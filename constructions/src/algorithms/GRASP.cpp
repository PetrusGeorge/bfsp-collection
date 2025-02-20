#include "algorithms/GRASP.h"
#include "Core.h"

GRASP::GRASP(Instance &instance) : m_instance(instance) {}

Solution GRASP::solve(double beta) {

    const size_t n = m_instance.num_jobs();     // num jobs
    const size_t m = m_instance.num_machines(); // num machines

    std::vector<std::vector<size_t>> d(1, std::vector<size_t>(m, 0)); // departure time matrix

    std::vector<size_t> seq = core::stpt_sort(m_instance); // sorting using the stpt rule

    std::vector<size_t> pi; // sequence

    pi.push_back(seq[0]); // getting the job with small processing time

    std::vector<bool> b_s(n, false); // vector bool to represent the jobs scheduled (true = scheduled)
    b_s[seq[0]] = true;

    for (size_t i = 1; i < n - 1; i++) {

        std::vector<size_t> rcl; // candidate list
        size_t cmin = std::numeric_limits<size_t>::max();
        size_t cmax = std::numeric_limits<size_t>::min();

        d = core::calculate_departure_times(m_instance, pi);

        pi.push_back({});
        std::vector<size_t> sigma;
        for (size_t j = 1; j < n; j++) {

            sigma.push_back(0);

            if (b_s[seq[j]]) {
                continue;
            }

            pi[i] = seq[j];

            // calculating the departure time of the job j that will be possibly inserted in the solution
            std::vector<size_t> new_departure_time = core::calculate_new_departure_time(m_instance, d, pi[i]);

            // computing the sum of the idle and blocking time of the job j
            sigma[sigma.size() - 1] = core::calculate_sigma(m_instance, d, new_departure_time, pi[i], i);

            // updating cmin or cmax
            if (sigma[sigma.size() - 1] < cmin) {
                cmin = sigma[sigma.size() - 1];
            } else if (sigma[sigma.size() - 1] > cmax) {
                cmax = sigma[sigma.size() - 1];
            }
        }

        // creating candidate list
        for (size_t j = 1; j < n; j++) {

            if ((double)sigma[j] <= (double)cmin + beta * (double)(cmax - cmin)) {
                rcl.push_back(seq[j]);
            }
        }

        // adding job in the sequence
        const size_t rand_idx = rand() % rcl.size();
        pi[pi.size() - 1] = rcl[rand_idx];
        b_s[rcl[rand_idx]] = true;
    }

    // adding last remaing job in the sequence
    for (size_t i = 0; i < n; i++) {
        if (!b_s[i]) {
            pi.push_back(i);
            break;
        }
    }

    // creating solution
    Solution s;
    s.sequence = pi;

    d = core::calculate_departure_times(m_instance, pi);
    s.departure_times = d;

    s.cost = d[n - 1][m - 1];

    return s;
}
