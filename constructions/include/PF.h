#ifndef PF_H
#define PF_H

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"
#include <functional>

class PF {
    public:
        PF();
        void STPT_Sort(Solution &sol);
        Solution solve(Solution &sol);
        Solution createSolutionFromInstance(const Instance &instance);
        
    private:
        // processing_times[j][k] is the processing time of job j on machine k.
        std::vector<std::vector<size_t>> processing_times;

};

std::vector<std::vector<size_t>> computeDepartureTimes(
    const std::vector<size_t>& sequence,
    const std::vector<std::vector<size_t>>& processing_times);

#endif