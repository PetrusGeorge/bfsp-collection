#ifndef DE_PLS_H
#define DE_PLS_H

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"
#include <functional>


class DE_PLS{
    public:
        DE_PLS();
        void STPT_Sort(Solution &sol);
        Solution PF(Solution &sol);
        Solution createSolutionFromInstance(const Instance &instance);

        // PF_NEH: gera x sequências candidatas e melhora os últimos δ jobs de cada uma
        Solution PF_NEH(int x, int delta);
        Solution GRASP_NEH(int gamma, int x);
        
    private:
        // processing_times[j][k] is the processing time of job j on machine k.
        std::vector<std::vector<size_t>> processing_times;

};  

std::vector<std::vector<size_t>> computeDepartureTimes(
    const std::vector<size_t>& sequence,
    const std::vector<std::vector<size_t>>& processing_times);

#endif