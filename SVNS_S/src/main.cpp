#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>

#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "SVNS_S.h"
#include "constructions/GRASP.h"
#include "constructions/LPT.h"
#include "constructions/MinMax.h"
#include "constructions/NEH.h"
#include "constructions/PF.h"
#include "constructions/PW.h"
#include "constructions/mNEH.h"
#include "local-search/RLS.h"

int main(int argc, char *argv[]) {

    Parameters params(argc, argv);
    Instance instance(params.instance_path());

    SVNS_S svns_s(instance, params);

    Solution solution_svns_s = svns_s.solve();

    std::cout << solution_svns_s.cost << '\n';
}
