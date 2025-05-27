#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "Instance.h"
#include "Log.h"
#include "MA.h"
#include "Parameters.h"
#include "RNG.h"

int main(int argc, char *argv[]) {

    Parameters params(argc, argv);
    Instance instance(params.instance_path());

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    DEBUG << "Showing normal debug macro\n";
    DEBUG_EXTRA << "Showing extra debug macro\n";

    VERBOSE(params.verbose()) << "Showing verbose macro\n";

    MA ma = MA(std::move(instance), std::move(params));
    const Solution best = ma.solve();
    std::cout << best.cost << '\n';
}
