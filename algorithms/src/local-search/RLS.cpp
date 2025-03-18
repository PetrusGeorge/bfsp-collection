#include "local-search/RLS.h"
#include "Instance.h"
#include "Solution.h"
#include "constructions/NEH.h"

#include <cassert>
#include <vector>

bool rls(Solution &s, const std::vector<size_t> &ref, Instance &instance) {

    bool improved = false;
    size_t j = 0;
    size_t cnt = 0;
    NEH helper(instance);
    while (cnt < instance.num_jobs()) {
        j = (j + 1) % instance.num_jobs();

        const size_t job = ref[j];
        for (size_t i = 0; i < s.sequence.size(); i++) {
            if (s.sequence[i] == job) {
                s.sequence.erase(s.sequence.begin() + (long)i);
            }
        }

        helper.set_taillard_matrices(s.sequence, job);
        auto [best_index, makespan] = helper.get_best_insertion();
        s.sequence.insert(s.sequence.begin() + (long)best_index, job);

        if (makespan < s.cost) {
            cnt = 0;
            s.cost = makespan;
            improved = true;
            continue;
        }

        cnt++;
    }

    return improved;
}
