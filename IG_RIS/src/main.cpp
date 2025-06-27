#include <cstdlib>
#include <iostream>

#include "IG_RIS.h"
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

    IG_RIS ig_ris(std::move(instance), std::move(params));
    const Solution best = ig_ris.solve();

    std::cout << best.cost << '\n';
}
