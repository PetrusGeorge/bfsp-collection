#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "Instance.h"
#include "Log.h"
#include "MFFO.h"
#include "Parameters.h"
#include "RNG.h"

int main(int argc, char *argv[]) {

    Parameters params(argc, argv);
    try {
        Instance instance(params.instance_path());
    } catch (std::runtime_error &err) {
        std::cerr << err.what() << '\n';
        exit(EXIT_FAILURE);
    }

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    std::cout << "Seed: " << RNG::instance().seed() << '\n';

    DEBUG << "Showing normal debug macro\n";
    DEBUG_EXTRA << "Showing extra debug macro\n";

    VERBOSE(params.verbose()) << "Showing verbose macro\n";

    MFFO mffo_instance(Instance(params.instance_path()), params);

    Solution s = MFFO::solve(mffo_instance);

    std::cout << s << '\n';
}
