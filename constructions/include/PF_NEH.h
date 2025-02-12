#ifndef PF_NEH_H
#define PF_NEH_H

#include "Instance.h"
#include "Parameters.h"
#include "Solution.h"
#include "PF.h"

class PF_NEH {
    public:
        PF_NEH();
        Solution solve(int x, int delta) ;
        
    private:
        // processing_times[j][k] is the processing time of job j on machine k.
        std::vector<std::vector<size_t>> processing_times;

};

#endif