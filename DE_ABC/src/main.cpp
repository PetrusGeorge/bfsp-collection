#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>

#include "DE_ABC.h"
#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"

int main(int argc, char *argv[]) {

    const Parameters params(argc, argv);
    Instance instance(params.instance_path());

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    // std::cout << "Seed: " << RNG::instance().seed() << '\n';

    DE_ABC deabc = DE_ABC(std::move(instance), params);
    const Solution s = deabc.solve();

    std::cout << s.cost << '\n';

    return 0;
}
