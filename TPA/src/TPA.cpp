#include "TPA.h"
#include "constructions/mNEH.h"

#include "Instance.h"
#include "Solution.h"

TPA::TPA(Instance &instance) : instance(instance) {}

Solution TPA::solve() {
    // 1st step: modified NEH
    Solution s_neh = MNEH::solve(0.8, instance);

    // 2nd step: simulated annealing
    SimulatedAnnealing sa(s_neh, instance, 1, 50000);
    Solution best = sa.solve();

    return best;
}
