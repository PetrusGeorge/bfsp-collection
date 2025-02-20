#ifndef NEH_H
#define NEH_H

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"
#include <functional>

class NEH {
  public:
    // Given a sequence phi, this function applies the NEH algorithm and returns a Solution object
    NEH(const std::vector<size_t> &phi, Instance &instance, Parameters &params);
    Solution solve();

  private:
    Instance &m_instance;
    Parameters &m_params;
    std::vector<size_t> m_phi;

    // TaillarDS (TDS) matrices
    std::vector<std::vector<size_t>> m_e; // Departure time
    std::vector<std::vector<size_t>> m_q; // Tail duration
    std::vector<std::vector<size_t>> m_f;

    // Sets all matrices, based on the sequence and the node
    // to be inserted k
    void set_taillard_matrices(const std::vector<size_t> &sequence, size_t k);

    // This function returns a pair with according to the populated TDS matrices
    // {best_index,resultant_makespan}
    std::pair<size_t, size_t> get_best_insertion();

    std::vector<std::vector<size_t>> calculate_departure_times(const std::vector<size_t> &sequence);

    // Pretty similiar to calculate departure times but everything is reversed,
    // jobs sequence, machines sequence and the matrix filling itself, used to
    // calculate the tail from taillard data structure
    std::vector<std::vector<size_t>> calculate_tail(const std::vector<size_t> &sequence);
};

#endif
