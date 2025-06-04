#include <cstdlib>
#include <iostream>

#include "Instance.h"
#include "P_EDA.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"

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

    P_EDA peda(instance, params, 50, 0.30);
    const Solution s_peda = peda.solve();
    std::cout << s_peda.cost << '\n';

    return 0;
}
