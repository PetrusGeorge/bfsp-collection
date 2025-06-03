#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "Instance.h"
#include "MFFO.h"
#include "Parameters.h"
#include "RNG.h"

int main(int argc, char *argv[]) {

    const Parameters params(argc, argv);
    try {
        const Instance instance(params.instance_path());
    } catch (std::runtime_error &err) {
        std::cerr << err.what() << '\n';
        exit(EXIT_FAILURE);
    }

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    MFFO mffo(Instance(params.instance_path()), params);

    const Solution s = mffo.solve();

    std::cout << s.cost << '\n';
}
