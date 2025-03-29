#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <numeric>

#include "Instance.h"
#include "P_EDA.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"
#include "constructions/GRASP.h"
#include "constructions/LPT.h"
#include "constructions/NEH.h"
#include "constructions/PF.h"
#include "constructions/PF_NEH.h"
#include "constructions/PW.h"
#include "constructions/mNEH.h"
#include "local-search/RLS.h"

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
    std::cout << "\nP_EDA:" << '\n';
    std::cout << s_peda << '\n';

   //  double media = 0;
   // for(size_t i = 1; i <= 10; i++){

   //      P_EDA peda(instance, params, 50, 0.30);
   //      const Solution s_peda = peda.solve();
   //      media += s_peda.cost;
   //  }
   //  std::cout << "\nP_EDA:" << '\n';
   //  std::cout << media/10 << '\n';
}
