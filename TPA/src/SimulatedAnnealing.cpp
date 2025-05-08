#include "constructions/NEH.h"
#include "SimulatedAnnealing.h"

#include "Core.h"
#include "Instance.h"
#include "Solution.h"
#include <utility>

SimulatedAnnealing::SimulatedAnnealing(Solution &solution, Instance &instance, double finalTemp, int nIter) : solution(solution), instance(instance), finalTemp(finalTemp), nIter(nIter) {}

Solution SimulatedAnnealing::solve() {
    calculateInitialTemp();
    calculateDecay();
    std::cout << initialTemp << std::endl;
    std::cout << decay << std::endl;

    return solution;
}

void SimulatedAnnealing::calculateInitialTemp() {
    long sum = 0;
    for (size_t i = 0; i < instance.num_jobs(); i++) {
        for (size_t j = 0; j < instance.num_machines(); j++) {
            sum += instance.p(i,j);
        }
    }

    initialTemp = sum/(5 * instance.num_jobs() * instance.num_machines());
}


void SimulatedAnnealing::calculateDecay() {
    double deltaTemp = finalTemp - initialTemp;

    decay = deltaTemp/((nIter - 1) * initialTemp * finalTemp);
}
