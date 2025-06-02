#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>

#include "HVNS.h"
#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"

int main(int argc, char *argv[]) {

    const Parameters params(argc, argv);
    Instance instance(params.instance_path());

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    HVNS hvns = HVNS(std::move(instance), params);
    const Solution best = hvns.solve();
    std::cout << best.cost << "\n";
}
