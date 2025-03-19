#include <cstdlib>
#include <iostream>

#include "RAIS.h"
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

    RAIS rais(std::move(instance), 0.97, 5, 4000, 6);
    const Solution best = rais.solve();

    std::cout << "custo: " << best.cost << "\n\n\n";
}
