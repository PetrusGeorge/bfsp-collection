#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>

#include "HDDE.h"
#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"

int main(int argc, char *argv[]) {

    const Parameters params(argc, argv);
    Instance instance(params.instance_path());

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    HDDE hdde = HDDE(std::move(instance), params);
    const Solution s = hdde.solve();

    std::cout << s.cost << '\n';

    return 0;
}