#include "HDDE.h"

std::vector<std::vector<size_t>> Mutation(std::vector<Solution>&, size_t);
void Crossover(std::vector<Solution>&,std::vector<std::vector<size_t>>); 

Solution HDDE(Instance &instance){
    std::vector<Solution> pop(instance.num_jobs());

    /*Step 1: Initialization phase. Set the parameters PS, CR, Z, and Pl .
    Initialize population. Let bestsofar be the best individual found so far,
    and t = 0.*/
    std::vector<size_t> phi(instance.num_jobs());
    std::iota(phi.begin(), phi.end(), 0);

    NEH n(instance); // false -> not reversed jobs
    Solution s_neh = n.solve(phi);
    std::cout << "\nNEH:" << '\n';
    std::cout << s_neh << '\n';

    /*Step 2: Mutation phase. Set t = t + 1, and generate PS mutant
    individuals by using the job-permutation-based mutation operator.*/
    std::vector<std::vector<size_t>> mut_pop = Mutation(pop, instance.num_jobs());

    /*Step 3: Crossover phase. Generate PS trial individuals by using the
    job-permutation-based crossover operator.*/
    Crossover(pop, mut_pop);

    /*Step 4: Local search phase. Perform the local search to Uit , i =
    1, 2, . . . , PS, with probability Pl.*/
    std::shuffle(phi.begin(), phi.end(), RNG::instance().gen());
    rls(s_neh, phi, instance);
    std::cout << "\nRLS:" << '\n';
    std::cout << s_neh << '\n';

    /*Step 5: Selection phase. Select PS target individuals by one-to-one
    selection operator for next generation.*/

    // Step 6: Update bestsofar.

    /*Step 7: If a stopping criterion is satisfied, then stop the procedure
    and output bestsofar; otherwise go back to Step 2.*/
}

std::vector<std::vector<size_t>> Mutation(std::vector<Solution>& pop, size_t n){
    // mutant scale factor
    double z = ((double)rand()/RAND_MAX);
    std::vector<std::vector<size_t>> mut_pop;

    // taking random solutions
    int i = rand()%pop.size();
    int j = rand()%pop.size();
    int k = rand()%pop.size();
    while(j == i) j = rand()%pop.size();
    while(k == i || k == j) k = rand()%pop.size();

    for(int y=0; y<pop.size(); y++){
        Solution a = pop[(i+y)%pop.size()], b = pop[(j+y)%pop.size()], c = pop[(k+y)%pop.size()];

        // di = xb - xc if rand() < Z, 0 otherwise.
        std::vector<size_t> sequencia(n, 0);
        for(int x=0; x<n; x++){
            if(((double)rand()/RAND_MAX) < z)
                sequencia[x] = b.sequence[x] - c.sequence[x];
        }

        // mod((xa + di + n), n)
        for(int x=0; x<n; x++){
            sequencia[x] = (a.sequence[x] + sequencia[x] + n)%n;
        }
        
        mut_pop.push_back(sequencia);
    }
}

void Crossover(std::vector<Solution> &pop, std::vector<std::vector<size_t>> mut_pop){
    for(int i=0; i<pop.size(); i++){
        Solution xi = pop[i];
        std::vector<size_t> mut_pop[i];

        
    }
}
