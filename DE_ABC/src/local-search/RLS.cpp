#include "local-search/RLS.h"
#include "Instance.h"
#include "Solution.h"
#include "constructions/NEH.h"

#include <cassert>
#include <vector>

// the common rls of bfsp collection is not being used
bool rls(Solution &s, Instance &instance) { // the original receive a ref

    bool improved = false;
    // size_t j = 0;
    size_t cnt = 0;
    NEH helper(instance);
    while (cnt < instance.num_jobs()) {
        // j = (j + 1) % instance.num_jobs();

        const size_t job = cnt; // ref[j]
        for (size_t i = 0; i < s.sequence.size(); i++) {
            if (s.sequence[i] == job) {
                s.sequence.erase(s.sequence.begin() + (long)i);
            }
        }

        auto [best_index, makespan] = helper.taillard_best_insertion(s.sequence, job);
        s.sequence.insert(s.sequence.begin() + (long)best_index, job);

        if (makespan < s.cost) {
            // cnt = 0;
            s.cost = makespan;
            improved = true;
            continue;
        }

        cnt++;
    }

    return improved;
}
