#ifndef NEH_H
#define NEH_H

#include "Instance.h"
#include "Solution.h"

class NEH {
  public:
    NEH(Instance &instance);

    // Given a sequence phi, this function applies the NEH algorithm and returns a
    // Solution object
    Solution solve(std::vector<size_t> phi);

    // This function returns a pair with according to the populated TDS matrices
    // {best_index,resultant_makespan}
    std::pair<size_t, size_t> taillard_best_insertion(const std::vector<size_t> &s, size_t k);

    std::pair<size_t, size_t> mtaillard_best_insertion(const std::vector<size_t> &s, size_t k, size_t original_position);

    void second_step(std::vector<size_t> phi, Solution &s);

  private:
    size_t insert_calculation(size_t i, size_t k, size_t best_value);

    Instance &m_instance;

    // TaillarDS (TDS) matrices
    // std::vector<std::vector<size_t>> m_e; // Departure time
    // std::vector<std::vector<size_t>> m_q; // Tail duration
    Solution m_inner; // Inner solution to avoid mallocs
};

#endif
