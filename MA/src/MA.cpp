#include "MA.h"
#include "Core.h"
#include "Instance.h"
#include "Log.h"
#include "RNG.h"
#include "Solution.h"

#include <iostream>
#include <vector>
#include <limits>
#include <bits/stdc++.h>
#include <chrono>
#include <ctime>

namespace {
  double uptime() {
      static const auto global_start_time = std::chrono::steady_clock::now();
      auto now = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - global_start_time);
      return static_cast<double>(duration.count());
}
}


std::vector<Solution> initial_pop(size_t ps) {
  
  size_t n = m_instance.num_jobs();

  std::vector<Solution> init_pop(ps);

  // init_pop[0] = PF_NEH(...);

  std::vector<size_t> antibody(n);
  std::iota(antibody.begin(), antibody.end(), 0);
  for(size_t i = 1; i < ps; i++) {

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count(); // new seed increase the randomness
    std::shuffle(antibody.begin(), antibody.end(), std::default_random_engine(seed));

    init_pop[i].first.sequence = antibody;
    core::recalculate_solution(m_instance, init_pop[i].first);

  }

  return init_pop;

}


size_t selection(std::vector<Solution> &pop) {

  size_t i = static_cast<size_t>(rand() % n);
  size_t j = i;

  while(i == j) j = static_cast<size_t>(rand() % n);

  if(pop[i].cost > pop[j].cost) {
    // delete pop[i] // really delete???
    return j;
  } else {
    // delete pop[j] // really delete???
    return i;
  }

}


void mutation(std::vector<Solution> &pop, double pm) {

  size_t n = m_instance.num_jobs();
  double p;

  for(size_t antibody = 0; antibody < pop.size(); antibody++) {

    p = rand() / static_cast<double>(RAND_MAX);

    if(p > pm) continue;
      // getting indexes for movements
    size_t i = static_cast<size_t>(rand() % n);
    size_t j = i;

    while(i == j) j = static_cast<size_t>(rand() % n);

    pop[antibody].sequence.insert(pop[antibody].sequence.begin()+i, pop[antibody].sequence[j]);
    
    if(i > j)
      pop[antibody].sequence.erase(pop[antibody].sequence.begin()+j); 
    else 
      pop[antibody].sequence.erase(pop[antibody].sequence.begin()+j+1); 
    
  
    core::recalculate_solution(m_instance, pop[antibody]);

  } 
  
}


void update(std::vector<std::pair<Solution, double>> &pop, std::vector<std::pair<Solution, double>> &clones) {
  // Search later:
  // R. Ruiz, C. Maroto, and J. Alcaraz, “Two new robust genetic algorithms
  // for the flowshop scheduling problem,” OMEGA, Int. J. Manage. Sci.,
  // vol. 34, pp. 461–476, 2006.
  std::vector<Solution> aux(nc);
  size_t i = 0, j = 0;

  for(size_t k = 0; k < nc; k++) {

    if(pop[i].second > clones[j].second) {
      aux[k] = pop[i];
      i++;
    } else {
      aux[k] = clones[j];
      j++;
    }

  }

  pop.swap(aux);

}


Solution solve() {

  srand(time(0));

  size_t n = m_instance.num_jobs();

  Solution best_solution;
  std::vector<Solution> pop;

  pop = initial_pop(); 

  std::sort(pop.begin(), pop.end(), [](Solution &p1, Solution &p2) { return p1.cost < p2.cost; }); // sorting by affinity (probably exist some best way to do this)
  
  best_solution = pop[0];

  // RLS(best_solution);

  // double timer_counter = 1;
  while(true) {

    while(pop.size() < PS) {

      size_t parent_1 = selection(pop);
      size_t parent_2 = parent_1;
      while(parent_2 == parent_1) parent_2 = selection(pop);

      double p = RNG::instance().generateDouble();

      size_t offspring1, offspring2;
      if(p < pc) {
        offspring1 = path_relink_swap(parent_1, parent_2);
        offspring2 = path_relink_swap(parent_2, parent_1);
      } else {
        offspring1 = parent_1;
        offspring2 = parent_2;
      }

    }

    p = RNG::instance().generateDouble();
    if(p < pm) {
      
    }

    if(pop[0].first.cost < best_solution.cost) {
      best_solution = pop[0].first;
      // std::cout << best_solution << std::endl;
    }

    if(uptime() > time_limit) break;
    // else if (uptime() > timer_counter * time_limit / 5) {
    //     std::cout << timer_counter * time_limit / 5 << " Microsseconds out of " << time_limit << '\n';
    //     timer_counter++;
    // }

  }

  return best_solution;

}