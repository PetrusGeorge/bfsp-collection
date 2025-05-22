#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>

// #include <vector>

#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "HVNS.h"


int main(int argc, char *argv[]) {

    // for(size_t a = 0; a < 9; a++) {
    //     for(size_t b = 0; b < 9; b++) {
    //         std::vector<int> sequence = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    //         size_t best_job = a;
    //         size_t best_index = b;
            
    //         if (best_job < best_index) {
    //             std::rotate(sequence.begin()+best_job, sequence.begin()+best_job+2, sequence.begin()+best_index+2);
    //         } else {
    //             std::rotate(sequence.begin()+best_index, sequence.begin()+best_job, sequence.begin()+best_job+2);
    //         }

    //         std::cout << "job index: " << a << "; insertion index: " << b << " >> "; 
    //         for(size_t i = 0; i < sequence.size(); i++) {
    //             std::cout << sequence[i] << " ";
    //         }
    //         std::cout << std::endl;
    //     }
    //     std::cout << std::endl;
    // }

    const Parameters params(argc, argv);
    Instance instance(params.instance_path());


    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    std::cout << "Seed: " << RNG::instance().seed() << '\n';
    
    HVNS hvns = HVNS(std::move(instance), std::move(params));
    Solution best = hvns.solve();
    std::cout << best.cost << "\n";
    
}
