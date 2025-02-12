#include <cstdlib>
#include <iostream>
#include <numeric>

#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "algorithms/LPT.h"
#include "algorithms/NEH.h"
#include "algorithms/PF.h"

int main(int argc, char *argv[]) {

    Parameters params(argc, argv);
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

    NEH n(phi, instance, params, false); // false -> not reversed jobs);
    const Solution s_neh = n.solve();
    std::cout << "\nNEH:" << '\n';
    std::cout << s_neh << '\n';

    const Solution s_lpt = LPT::solve(instance);
    std::cout << "\nLPT:" << '\n';
    std::cout << s_lpt << '\n';

    Solution s_pf;
    s_pf = PF::solve(instance);

    std::cout << "\nPF:" << '\n';
    std::cout << s_pf << '\n';
}
