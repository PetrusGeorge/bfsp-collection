#include <cstdlib>
#include <iostream>
#include <numeric>

#include "Instance.h"
#include "algorithms/LPT.h"
#include "Log.h"
#include "algorithms/NEH.h"
#include "algorithms/PF.h"
#include "Parameters.h"
#include "RNG.h"

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

    DEBUG << "Showing normal debug macro\n";
    DEBUG_EXTRA << "Showing extra debug macro\n";

    VERBOSE(params.verbose()) << "Showing verbose macro\n";

    std::vector<size_t> phi(instance.num_jobs());
    std::iota(phi.begin(), phi.end(), 0);

    NEH n(phi, instance, params, false); // false -> not reversed jobs);
    const Solution s_neh = n.solve();
    std::cout << "\nNEH:" << '\n';
    std::cout << s_neh << '\n';

    LPT l(instance, params, false); // false -> not reversed jobs);
    const Solution s_lpt = l.solve();
    std::cout << "\nLPT:" << '\n';
    std::cout << s_lpt << '\n';

    PF pf;
    Solution s_pf = pf.createSolutionFromInstance(instance);
    pf.STPT_Sort(s_pf);
    s_pf = pf.solve(s_pf);

    std::cout << "\nPF:" << '\n';
    std::cout << s_pf << '\n';
}
