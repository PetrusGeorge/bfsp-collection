#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "SVNS_D.h"

int main(int argc, char *argv[]) {

    Parameters params(argc, argv);
    Instance instance(params.instance_path());

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }
    std::cout << "Seed: " << RNG::instance().seed() << '\n';

    SVNS_D svns_d(instance, params);

    Solution solution_svns_d = svns_d.solve();
}
