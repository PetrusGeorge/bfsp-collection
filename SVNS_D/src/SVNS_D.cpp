#include "SVNS_D.h"

#include "constructions/NEH.h"
#include "constructions/PW.h"

SVNS_D::SVNS_D(Instance &instance, Parameters &params) : m_instance(instance), m_parameters(params) {}

Solution SVNS_D::PW_PWE2() {
    Solution solution;

    PW pw(m_instance);
    NEH neh(m_instance);

    Solution pw_solution = pw.solve();

    Solution pwe_solution = neh.solve(pw_solution.sequence);

    if (pwe_solution.cost < pw_solution.cost) {
        solution = pwe_solution;
    } else {
        solution = pw_solution;
    }
    return solution;
}

Solution SVNS_D::solve() {
    Solution solution = PW_PWE2();

    return solution;
}
