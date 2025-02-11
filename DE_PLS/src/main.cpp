#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "Instance.h"
#include "Log.h"
#include "Parameters.h"
#include "RNG.h"
#include "DE_PLS.h"

int main(int argc, char *argv[]) {

    Parameters params(argc, argv);
    Instance instance(params.instance_path());

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    std::cout << "Seed: " << RNG::instance().seed() << '\n';

    DEBUG << "Showing normal debug macro\n";
    DEBUG_EXTRA << "Showing extra debug macro\n";

    std::cout << "Instance Matrix:\n";
    for (size_t i = 0; i < instance.num_jobs(); ++i) {
        for (size_t j = 0; j < instance.num_machines(); ++j) {
            std::cout << instance.p(i, j) << ' ';
        }
        std::cout << '\n';
    }

    // DEPLS 

    DE_PLS depls;

    Solution sol = depls.createSolutionFromInstance(instance);
    sol = depls.PF(sol);

    std::cout << "Sequência PF: ";
    for (size_t job : sol.sequence)
        std::cout << job << " ";
    std::cout << "\nMakespan (cost): " << sol.cost << "\n";

    VERBOSE(params.verbose()) << "Showing verbose macro\n";
}
