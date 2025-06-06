#ifndef HMGHS_H
#define HMGHS_H

#include "Core.h"
#include "Instance.h"
#include "Log.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"
#include "constructions/NEH.h"
#include "local-search/RLS.h"

#include <vector>

class hmgHS {
  public:
    hmgHS(Instance instance, Parameters params);

    void check_minmax(Solution &s);

    // Generate 2 random sequence using NEH and NEH-WPT and other (MS-2) randomly
    void generate_initial_pop();

    std::vector<size_t> generate_random_sequence();

    // Generate a random harmony (vector of double that can be converted on a job permutation)
    void generate_random_harmony(Solution &sol);

    // convert a permutation into an harmony
    void permutation_to_harmony(Solution &s);

    // convert an harmony into a job permutation
    void harmony_to_permutation(Solution &s);

    // return the minimum and the maximum value for index j found between all harmonies of the population
    std::pair<double, double> get_min_max_of_position(size_t j);

    // create a new harmony using the other ones as inspiration
    void improvise_new_harmony(Solution &s);

    // This function deals with job with the same value on the harmony
    void revision(Solution &s);

    // rebuilds the harmony from the job permutation generated by RLS
    void sort_permutation(Solution &s);

    // Improve the population
    void update(Solution &s);

    // apply the hmgHS method
    Solution solve();

  private:
    Instance m_instance;
    Parameters m_params;
    size_t m_time_limit;
    std::vector<Solution> m_pop;
    std::vector<double> m_min;
    std::vector<double> m_max;
};

#endif