#include "algorithms/PW.h"
#include "Core.h"

#include <cmath>
#include <numeric>

PW::PW(Instance &instance) : m_instance(instance) {}

std::vector<double> PW::calculate_avg_processing_time(size_t candidate_job, std::vector<size_t> &unscheduled) {

    const size_t m = m_instance.num_machines(); // number of machines

    std::vector<double> artificial_times(m, 0); // m processing times

    auto p = [this](size_t i, size_t j) { return m_instance.p(i, j); };

    for (const unsigned long job : unscheduled) {

        if (job == candidate_job) {
            continue; // avoiding the candidate job
        }

        // getting the sum of the processing time on each machine for each job
        for (size_t machine = 0; machine < m; machine++) {
            artificial_times[machine] += (double)p(job, machine);
        }
    }

    // dividing each processing time by (n - k - 1)
    for (size_t i = 0; i < m; i++) {
        artificial_times[i] /= ((double)unscheduled.size() - 1);
    }

    return artificial_times;
}

void PW::update_avg_processing_time(const size_t previous_job, const size_t next_job, const size_t qt_unscheduled,
                                    std::vector<double> &artificial_processing_times) {

    const size_t m = m_instance.num_machines(); // number of machines
    auto p = [this](size_t i, size_t j) { return m_instance.p(i, j); };

    for (size_t machine = 0; machine < m; machine++) {
        artificial_processing_times[machine] *= (double)(qt_unscheduled - 1);
    }

    for (size_t machine = 0; machine < m; machine++) {
        artificial_processing_times[machine] += (double)p(previous_job, machine);
    }

    for (size_t machine = 0; machine < m; machine++) {
        artificial_processing_times[machine] -= (double)p(next_job, machine);
    }

    for (size_t machine = 0; machine < m; machine++) {
        artificial_processing_times[machine] /= (double)(qt_unscheduled - 1);
    }
}

std::vector<double> PW::calculate_artificial_departure_time(std::vector<std::vector<size_t>> &d,
                                                            std::vector<double> &artificial_processing_times) {

    const size_t m = m_instance.num_machines(); // number of machines

    std::vector<double> artificial_departure_time(m);

    artificial_departure_time[0] =
        std::max(((double)d[d.size() - 1][0]) + artificial_processing_times[0], (double)d[d.size() - 1][1]);

    for (size_t j = 1; j < m - 1; j++) {

        const double current_finish_time = artificial_departure_time[j - 1] + artificial_processing_times[j];

        artificial_departure_time[j] = std::max(current_finish_time, (double)d[d.size() - 1][j + 1]);
    }

    artificial_departure_time.back() = artificial_departure_time[m - 2] + artificial_processing_times[m - 1];

    return artificial_departure_time;
}

double PW::calculate_chi(std::vector<size_t> &new_departure_time, std::vector<double> &artificial_departure_time,
                         std::vector<double> &artificial_processing_times) {

    const size_t m = m_instance.num_machines(); // number of machines

    double chi = 0;

    for (size_t machine = 0; machine < m; machine++) {

        const double sum = (artificial_departure_time[machine] - (double)new_departure_time[machine] -
                            artificial_processing_times[machine]);

        chi += sum;
    }

    return chi;
}

double PW::calculate_f(std::vector<std::vector<size_t>> &d, std::vector<size_t> &new_departure_time, double chi,
                       size_t job, size_t k) {

    const size_t n = m_instance.num_jobs(); // number of jobs
    const size_t sigma = core::calculate_sigma(m_instance, d, new_departure_time, job, k);

    const double f = ((double)(n - k - 2) * (double)sigma + chi);

    return f;
}

Solution PW::solve() {

    const size_t n = m_instance.num_jobs();     // number of jobs
    const size_t m = m_instance.num_machines(); // number of machines

    std::vector<size_t> new_seq;        // partial sequence
    std::vector<size_t> unscheduled(n); // list of unscheduled jobs (initially 0, 1, ..., n)
    std::iota(unscheduled.begin(), unscheduled.end(), 0);

    std::vector<double> artificial_processing_times; // processing time of the artificial job v
    std::vector<double> artificial_departure_time;   // departure time of the artificial job v
    std::vector<size_t> new_departure_time;          // hipotetical departure time of the job j

    size_t best_i = std::numeric_limits<size_t>::max();            // store the index of the variable with smallest f
    double smallest_f = std::numeric_limits<double>::infinity();   // smallest f
    double smallest_chi = std::numeric_limits<double>::infinity(); // smallest chi

    double chi = NAN;
    double f = NAN;

    std::vector<std::vector<size_t>> d_current; // current departure time

    // allocating the first job

    new_seq.push_back({});
    artificial_processing_times = calculate_avg_processing_time(0, unscheduled);
    for (size_t i = 0; i < n; i++) {
        new_seq[0] = i;

        d_current = core::calculate_departure_times(m_instance, new_seq);

        artificial_departure_time = calculate_artificial_departure_time(d_current, artificial_processing_times);

        chi = calculate_chi(d_current[0], artificial_departure_time, artificial_processing_times);

        f = calculate_f(d_current, d_current[0], chi, i, 0);

        if (f < smallest_f || (f == smallest_f && chi < smallest_chi)) {
            best_i = i;
            smallest_chi = chi;
            smallest_f = f;
        }

        if (i + 1 == n) {
            break;
        }

        update_avg_processing_time(i, i + 1, unscheduled.size(), artificial_processing_times);
    }

    new_seq[0] = best_i;
    unscheduled.erase(unscheduled.begin() + (long)best_i);

    ////////////////////////////////////////////////////

    for (size_t k = 1; k <= n - 2; k++) {

        d_current = core::calculate_departure_times(m_instance, new_seq);

        best_i = std::numeric_limits<size_t>::max();
        smallest_chi = std::numeric_limits<double>::infinity();
        smallest_f = std::numeric_limits<double>::infinity();

        artificial_processing_times = calculate_avg_processing_time(unscheduled[0], unscheduled);
        for (size_t i = 0; i < unscheduled.size(); i++) {

            artificial_departure_time = calculate_artificial_departure_time(d_current, artificial_processing_times);
            new_departure_time = core::calculate_new_departure_time(m_instance, d_current, unscheduled[i]);

            chi = calculate_chi(new_departure_time, artificial_departure_time, artificial_processing_times);
            f = calculate_f(d_current, new_departure_time, chi, unscheduled[i], k);

            if (f < smallest_f || (f == smallest_f && chi < smallest_chi)) {
                best_i = i;
                smallest_chi = chi;
                smallest_f = f;
            }

            if (i + 1 == unscheduled.size()) {
                break;
            }

            update_avg_processing_time(unscheduled[i], unscheduled[i + 1], unscheduled.size(),
                                       artificial_processing_times);
        }

        new_seq.push_back(unscheduled[best_i]);
        unscheduled.erase(unscheduled.begin() + (long)best_i);
    }

    new_seq.push_back(unscheduled[0]);

    Solution s;
    s.sequence = new_seq;

    d_current = core::calculate_departure_times(m_instance, new_seq);
    s.departure_times = d_current;

    s.cost = d_current[n - 1][m - 1];

    return s;
}
