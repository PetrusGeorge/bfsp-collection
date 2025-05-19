#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "DIWO.h"
#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"

int main(int argc, char *argv[]) {

    Parameters params(argc, argv);

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    try {
        Instance instance(params.instance_path());
        DIWO diwo(std::move(instance), std::move(params));
        std::cout << diwo.solve().cost << '\n';
    } catch (std::runtime_error &err) {
        std::cerr << err.what() << '\n';
        exit(EXIT_FAILURE);
    }
}
