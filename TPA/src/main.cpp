#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>

#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "TPA.h"

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

    std::vector<size_t> phi(instance.num_jobs());
    std::iota(phi.begin(), phi.end(), 0);

    // MNEH n;
    // Solution s_neh = n.solve(0.8, instance);
    // std::cout << "\nNEH:" << '\n';
    // std::cout << s_neh << '\n';

    TPA tpa(instance);
    Solution s_tpa = tpa.solve();
    std::cout << s_tpa << std::endl;
}
