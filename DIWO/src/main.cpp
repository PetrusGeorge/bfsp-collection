#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "DIWO.h"
#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"

int main(int argc, char *argv[]) {

    const Parameters params(argc, argv);
    const DIWOParams aparams;

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    try {
        Instance instance(params.instance_path());
        DIWO diwo(std::move(instance), aparams);
        std::cout << diwo.solve() << '\n';
    } catch (std::runtime_error &err) {
        std::cerr << err.what() << '\n';
        exit(EXIT_FAILURE);
    }
}
