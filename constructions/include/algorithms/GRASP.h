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

    // construct a solution using the greed GRASP criterion
    Solution solve(double beta);

  private:
		const Instance *instance;

};


#endif 
