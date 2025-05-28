#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <numeric>

#include "Core.h"
#include "DE_PLS.h"
#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"

int main(int argc, char *argv[]) {

    const Parameters params(argc, argv);
    Instance instance(params.instance_path());

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    DE_PLS depls(std::move(instance), params);
    const Solution s = depls.solve();

    std::cout << s.cost << '\n';
}
