#include "constructions/PF_NEH.h"
#include "Instance.h"
#include "constructions/NEH.h"
#include "constructions/PF.h"

PF_NEH::PF_NEH(Instance &instance) : m_instance(instance) {};

Solution PF_NEH::solve(size_t lambda) {

    const size_t n = m_instance.num_jobs();

    PF pf(m_instance);
    NEH neh(m_instance);

    // generate initial solution with PF algorithm
    Solution s = pf.solve();

    // apply the NEH algorithm with 0 to n - lambda jobs from the PF solution
    const std::vector<size_t> candidate_jobs = {s.sequence.begin() + (long)(n - lambda + 1), s.sequence.end()};
    s.sequence = {s.sequence.begin(),
                  s.sequence.begin() +
                      (long)(n - lambda + 1)}; // needs to be n - lambda + 1 because [first_pos, last_post)

    neh.second_step(candidate_jobs, s);

    return s;
}
