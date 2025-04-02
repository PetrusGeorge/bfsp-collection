#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>

#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "hmgHS.h"


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

    hmgHS hmgHS_solver = hmgHS(std::move(instance), std::move(params));
    const Solution best = hmgHS_solver.solve();
    std::cout << "custo: " << best.cost << "\n\n\n";
    // std::vector<size_t> phi(instance.num_jobs());
    // std::iota(phi.begin(), phi.end(), 0);

    // NEH n(instance); // false -> not reversed jobs);
    // Solution s_neh = n.solve(phi);
    // std::cout << "\nNEH:" << '\n';
    // std::cout << s_neh << '\n';

    // std::shuffle(phi.begin(), phi.end(), RNG::instance().gen());
    // rls(s_neh, phi, instance);
    // std::cout << "\nRLS:" << '\n';
    // std::cout << s_neh << '\n';
    
}
