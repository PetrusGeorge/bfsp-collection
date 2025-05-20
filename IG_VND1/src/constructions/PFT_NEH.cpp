#include "constructions/PFT_NEH.h"
#include "Instance.h"
#include "constructions/NEH.h"
#include "constructions/PFT.h"

PFT_NEH::PFT_NEH(Instance &instance) : m_instance(instance) {};

Solution PFT_NEH::solve(size_t lambda) {

    const size_t n = m_instance.num_jobs();

    PFT pft(m_instance);

    NEH neh(m_instance);

    // generate initial solution with PFT algorithm
    Solution s = pft.solve();

    // apply the NEH algorithm with 0 to n - lambda jobs from the PF solution
    const std::vector<size_t> candidate_jobs = {s.sequence.begin() + (long)(n - lambda + 1), s.sequence.end()};
    s.sequence = {s.sequence.begin(),
                  s.sequence.begin() +
                      (long)(n - lambda + 1)}; // needs to be n - lambda + 1 because [first_pos, last_post)

    neh.second_step(candidate_jobs, s);

    return s;
}
