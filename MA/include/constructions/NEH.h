#ifndef NEH_H
#define NEH_H

#include "Instance.h"
#include "Solution.h"

class NEH {
  public:
    // Given a sequence phi, this function applies the NEH algorithm and returns a
    // Solution object
    NEH(Instance &instance);
    Solution solve(std::vector<size_t> phi);

    // This function returns a pair with according to the populated TDS matrices
    // {best_index,resultant_makespan}
    std::pair<size_t, size_t> taillard_best_insertion(const std::vector<size_t> &sequence, size_t pos);

    void second_step(std::vector<size_t> phi, Solution &s);

  private:
    Instance &m_instance;

    // TaillarDS (TDS) matrices
    std::vector<std::vector<size_t>> m_e; // Departure time
    std::vector<std::vector<size_t>> m_q; // Tail duration
    std::vector<std::vector<size_t>> m_f;
};

#endif