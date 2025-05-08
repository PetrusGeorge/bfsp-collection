#include "constructions/NEH.h"
#include "SimulatedAnnealing.h"

#include "Core.h"
#include "Instance.h"
#include "Solution.h"
#include "RNG.h"
#include <utility>
#include <cmath>   

SimulatedAnnealing::SimulatedAnnealing(Solution &solution, Instance &instance, double finalTemp, int nIter) : solution(solution), instance(instance), finalTemp(finalTemp), nIter(nIter) {}

Solution SimulatedAnnealing::solve() {
    calculateInitialTemp();
    calculateDecay();

    double currentTemp = initialTemp;
    double referenceCost = solution.cost;
    double bestCost = solution.cost;

    int nJobs = instance.num_jobs();

    Solution bestSolution = solution;
    Solution currentSolution = solution;



    while (currentTemp >= finalTemp) {
        int node = RNG::instance().generate<int>(0, nJobs);
        Solution newSol = anneal(currentSolution, node);
        core::recalculate_solution(instance, newSol);
        double delta = newSol.cost - currentSolution.cost;

        if (delta <= 0) {
            currentSolution = newSol;
            referenceCost = newSol.cost;

            if (referenceCost < bestCost) {
                bestSolution = newSol;
                bestCost = referenceCost;
            }
        }
        else {
            double acceptanceProbability = std::exp(-delta/T);
            double random = RNG::instance().generate<double>(0, 1);

            if (acceptanceProbability > random) {
                currentSolution = newSol;
                referenceCost = newSol.cost;
            }
        }

        currentTemp = currentTemp/(1 + (decay * currentTemp));

    }

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

Solution SimulatedAnnealing::anneal(Solution &currentSolution, int nodeToChange) {

}


