#ifndef Grasp_H
#define Grasp_H

#include <iostream>
#include <vector>
#include <limits>
#include <numeric>
#include "Core.h"
#include "Instance.h"
#include "Solution.h"

class GRASP {

  public:
    GRASP();
    GRASP(const Instance &instance);

    ~GRASP();

    // sort the jobs
    // std::vector<size_t> stpt_sort(const Instance &instance); 

    // calculate the departure time of a job if it were inserted into the solution
    // std::vector<size_t> computeNewDepartureTime(std::vector<std::vector<size_t>> &d, size_t node);

    // calculate the sum of idle and blocking time
    // double computeSigma(std::vector<std::vector<size_t>> &d, std::vector<size_t> &newDepartureTime, size_t job, double k);

    // construct a solution using the greed GRASP criterion
    Solution solve(double beta);

  private:
		const Instance *instance;

};


#endif 
