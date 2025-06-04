#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>

#include "Solution.h"
#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "IG_VND2.h"

int main(int argc, char *argv[]) {

    Parameters params(argc, argv);
    Instance instance(params.instance_path());

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    IG_VND2 ig_vnd(std::move(instance), std::move(params));
    const Solution best = ig_vnd.solve();

    std::cout << best.cost << '\n';
    
}
