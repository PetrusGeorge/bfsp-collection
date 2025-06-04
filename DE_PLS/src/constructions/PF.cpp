#include "constructions/PF.h"
#include "Core.h"

#include <algorithm>
#include <cassert>
#include <vector>

PF::PF(Instance &instance) : m_instance(instance) {}

//---------------------------------------------------------
// PF heuristic implementation.
// This function builds a new job sequence by selecting, at each iteration,
// the candidate job (among unscheduled jobs) that minimizes the sum of idle and
// blocking times. (Here we use a measure sigma computed from the difference in
// departure times.)
Solution PF::solve() {

    Solution sol;

    // sorting by smallest total processing time
    std::vector<size_t> stpt = core::stpt_sort(m_instance);

    pf_insertion_phase(sol, stpt.front());

    return sol;
}

void PF::pf_insertion_phase(Solution &s, size_t first_job) {
    const size_t n = m_instance.num_jobs();     // número de jobs

    // selects job with smallest total processing time to be the first one
    std::vector<bool> scheduled(n, false);
    std::vector<size_t> new_seq;
    std::vector<size_t> unscheduled;
    new_seq.push_back(first_job);
    scheduled[first_job] = true;

    // selecting unscheduled jobs, i.e. all jobs besides the first one
    for (size_t i = 0; i < n; ++i) {
        if (!scheduled[i]) {
            unscheduled.push_back(i);
        }
    }

    // general idea:
    // starts sequence with one job
    // test the insertion of all unscheduled jobs at the end of the current sequence
    // to compare the insertions, the sigma(j,k), j is the job, and k the sequence position
    // sigma(j,k) represents the sum of idle and blocking times from adding job j to position k+1
    // after computing sigma for all jobs in the unscheduled vector
    // add the job with the lowest sigma to the main sequence
    Solution s_current;
    s_current.sequence = new_seq;
    Solution s_candidate;
    for (size_t k = 1; k < n - 1; ++k) {

        core::recalculate_solution(m_instance, s_current);
        size_t best_sigma = std::numeric_limits<size_t>::max();
        size_t best_job = unscheduled.front();

        for (const size_t candidate : unscheduled) {
            std::vector<size_t> candidate_sequence = new_seq;
            candidate_sequence.push_back(candidate);
            s_candidate.sequence = candidate_sequence;

            core::recalculate_solution(m_instance, s_candidate);
            std::vector<size_t> &d_new = s_candidate.departure_times.back();

            // computing sigma(j,k) criterium
            const size_t sigma = core::calculate_sigma(m_instance, s_current.departure_times, d_new, candidate);
            if (sigma < best_sigma) {
                best_sigma = sigma;
                best_job = candidate;
            }
        }

        new_seq.push_back(best_job);
        scheduled[best_job] = true;
        unscheduled.erase(std::remove(unscheduled.begin(), unscheduled.end(), best_job), unscheduled.end());
    }
    new_seq.push_back(unscheduled.front());

    s.sequence = new_seq;
    core::recalculate_solution(m_instance, s);
}
