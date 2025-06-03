#include "TPA.h"
#include "constructions/mNEH.h"

#include "Instance.h"
#include "Solution.h"

TPA::TPA(Instance &instance, Parameters &params) : instance(instance), params(params) {}

Solution TPA::solve() {
    // 1st step: modified NEH
    Solution s_neh = MNEH::solve(params.alpha(), instance);

    // 2nd step: simulated annealing
    SimulatedAnnealing sa(s_neh, instance, params);
    Solution best = sa.solve();

    return best;
}
