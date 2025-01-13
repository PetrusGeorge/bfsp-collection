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
    // This is a implementation of the mme2 algorithm, a variation of neh using min max as the first step
    Solution initial_solution();

    // Min Max algorithm, used to substitute LPT for the first step of neh
    std::vector<size_t> min_max(bool jobs_reversed = false);

    // neh functions owns phi, if you want to avoid copies call neh with std::move on this argument
    // The neh algorithm without the implementation of the first step, so you can use lpt, min max, etc,
    // just need put the output of those algorithms on the phi parameter
    Solution neh(std::vector<size_t> phi, bool jobs_reversed = false);

    // This function is called by the neh function, but can also be used standalone
    void neh_second_step(std::vector<size_t> phi, Solution &s, bool jobs_reversed = false);

    // Easy hack to implement algorithms using both direct and reverse instances
    std::function<long(size_t, size_t)> get_reversible_matrix(bool jobs_reversed);

    // Pretty straight foward local search
    Solution local_search(Solution s);
    // The name also implies here
    bool swap_first_improvement(Solution &s);

    // Random destroy solution using the parameter d defined on the params struct
    std::vector<size_t> destroy(Solution &s);

    // Set both departure times and cost for the solution based on the currect sequence, no verifications are done
    // incomplete and wrong sequences are not going to be reported
    void recalculate_solution(Solution &s);
    std::vector<std::vector<size_t>> calculate_departure_times(const std::vector<size_t> &sequence,
                                                               bool jobs_reversed = false);
    // Pretty similiar to calculate departure times but everything is reversed, jobs sequence, machines sequence
    // and the matrix filling itself, used to calculate the tail from taillard data structure
    std::vector<std::vector<size_t>> calculate_tail(const std::vector<size_t> &sequence, bool jobs_reversed = false);

    // These classes are "imutable" all their members are private and there is no "set" method for those members
    // almost like a read only memory
    Instance m_instance;
    Parameters m_params;

    // Taillard data structure for bfsp neh
    struct TaillardDS {
        std::vector<std::vector<size_t>> e; // Departure time
        std::vector<std::vector<size_t>> q; // Tail duration
        std::vector<std::vector<size_t>> f;
    };

    // Returns a TaillarDS with all matrices set, based on the sequence and the node to be inserted k
    TaillardDS get_taillard(const std::vector<size_t> &sequence, size_t k, bool jobs_reversed);

    // This function receives a tds that must be already properly populated (get_taillard function)
    // and returns a pair with {best_index,resultant_makespan}
    static std::pair<size_t, size_t> get_best_insertion(const TaillardDS &tds);
};

#endif
