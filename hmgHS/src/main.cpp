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

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    hmgHS hmgHS_solver = hmgHS(std::move(instance), std::move(params));
    const Solution best = hmgHS_solver.solve();
    std::cout << best.cost << '\n';
}
