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

    /*try {*/
    /*    Instance instance(params.instance_path());*/
    /*} catch (std::runtime_error &err) {*/
    /*    std::cerr << err.what() << '\n';*/
    /*    exit(EXIT_FAILURE);*/
    /*}*/

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    std::cout << "Seed: " << RNG::instance().seed() << '\n';

    HDDE hdde = HDDE(std::move(instance), std::move(params));
    Solution s = hdde.solve();

    std::cout << "custo: " << s.cost << "\n\n\n";

    return 0;
}