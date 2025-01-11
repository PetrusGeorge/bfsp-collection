#ifndef IG_H
#define IG_H

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"
#include <functional>

class IG {
  public:
    IG(Instance instance, Parameters params);
    Solution solve();

  private:
    Solution initial_solution();
    std::vector<size_t> min_max(bool jobs_reversed = false);

    // neh functions owns phi, if you want to avoid copies call neh with std::move on this argument
    Solution neh(std::vector<size_t> phi, bool jobs_reversed = false);
    void neh_second_step(std::vector<size_t> phi, Solution &s, bool jobs_reversed = false);

    Solution mme2();
    std::function<long(size_t, size_t)> get_reversible_matrix(bool jobs_reversed);

    Solution local_search(Solution s);
    bool swap_first_improvement(Solution &s);
    std::vector<size_t> destroy(Solution &s);

    void recalculate_solution(Solution &s);
    std::vector<std::vector<size_t>> calculate_departure_times(const std::vector<size_t> &sequence,
                                                               bool jobs_reversed = false);
    // Pretty similiar to calculate departure times but everything is reversed, jobs sequence, machines sequence
    // and the matrix filling itself, used to calculate the tail from taillard data structure
    std::vector<std::vector<size_t>> calculate_tail(const std::vector<size_t> &sequence, bool jobs_reversed = false);

    Instance m_instance;
    Parameters m_params;

    Solution m_best;
    // Taillard data structure for bfsp neh
    struct TaillardDS {
        std::vector<std::vector<size_t>> e; // Departure time
        std::vector<std::vector<size_t>> q; // Tail duration
        std::vector<std::vector<size_t>> f;
    };

    TaillardDS get_taillard(const std::vector<size_t> &sequence, size_t k, bool jobs_reversed);

    // This function receives a tds that must be already properly populated
    // and returns a pair with {best_index,resultant_makespan}
    static std::pair<size_t, size_t> get_best_insertion(const TaillardDS &tds);
};

#endif
