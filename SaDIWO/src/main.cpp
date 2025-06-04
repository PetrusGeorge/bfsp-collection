#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "Instance.h"
#include "Log.h"
#include "Parameters.h"
#include "RNG.h"
#include "SaDIWO.h"

int main(int argc, char *argv[]) {

    Parameters params(argc, argv);
    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    try {
        Instance instance(params.instance_path());
        SaDIWO sadiwo(std::move(instance), std::move(params));
        std::cout << sadiwo.solve().cost << "\n";
    } catch (std::runtime_error &err) {
        std::cerr << err.what() << '\n';
        exit(EXIT_FAILURE);
    }

    DEBUG << "Showing normal debug macro\n";
    DEBUG_EXTRA << "Showing extra debug macro\n";

    VERBOSE(params.verbose()) << "Showing verbose macro\n";
}
