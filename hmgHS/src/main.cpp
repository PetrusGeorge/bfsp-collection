#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>

#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "hmgHS.h"

int main(int argc, char *argv[]) {

    const Parameters params(argc, argv);
    Instance instance(params.instance_path());

    /*try {*/
    /*    Instance instance(params.instance_path());*/
    /*} catch (std::runtime_error &err) {*/
    /*    std::cerr << err.what() << '\n';*/
    /*    exit(EXIT_FAILURE);*/
    /*}*/

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    // std::cout << "Seed: " << RNG::instance().seed() << '\n';

    hmgHS hmgHS_solver = hmgHS(std::move(instance), std::move(params));
    const Solution best = hmgHS_solver.solve();
    std::cout << "cost: " << best.cost << "\n\n\n";
}
