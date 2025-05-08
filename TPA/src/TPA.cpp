#include "constructions/NEH.h"
#include "constructions/mNEH.h"
#include "TPA.h"

#include "Core.h"
#include "Instance.h"
#include "Solution.h"
#include <utility>

TPA::TPA(Instance &instance) : instance(instance) {}

Solution TPA::solve() {
    // 1st step: modified NEH
    MNEH n;
    Solution s_neh = n.solve(0.8, instance);

    //2nd step: simulated annealing
    SimulatedAnnealing sa(s_neh, instance, 1, 50000);
    sa.solve();

    return s_neh;

}
