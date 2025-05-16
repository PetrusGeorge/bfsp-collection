#include <cstdlib>
#include <iostream>

#include "IG.h"
#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"

int main(int argc, char *argv[]) {

    Parameters params(argc, argv);
    Instance instance(params.instance_path());

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    IG ig(std::move(instance), std::move(params));
    const Solution best = ig.solve();

    std::cout << best.cost << '\n';
}
