#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>

#include "Solution.h"
#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "IG_VND1.h"

int main(int argc, char *argv[]) {

    Parameters params(argc, argv);
    Instance instance(params.instance_path());

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }
    
    IG_VND1 ig_vnd1(std::move(instance), std::move(params));
    const Solution best = ig_vnd1.solve();

    std::cout << best.cost << '\n';
    
}
