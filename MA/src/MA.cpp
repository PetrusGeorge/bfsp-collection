#include "MA.h"

#include <iostream>
#include <algorithm>
#include <limits>
#include <chrono>

namespace {
  size_t uptime() {
    static const auto global_start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - global_start_time);
    return duration.count();
}
}

MA::MA(Instance instance, Parameters params) 
  : m_instance(std::move(instance)), m_params(std::move(params)) {
    this->time_limit = 5 * m_instance.num_jobs() * m_instance.num_machines();

}


void MA::generate_initial_pop() {
  
  size_t n = m_instance.num_jobs();
  size_t lambda = n > LAMBDA_MAX ? LAMBDA_MAX : n; 

  pop = std::vector<Solution>(m_params.ps());

  PF_NEH pf_neh(m_instance);  
  pop[0] = pf_neh.solve(lambda);
  core::recalculate_solution(m_instance, pop[0]);

  std::vector<size_t> individual(n);
  std::iota(individual.begin(), individual.end(), 0);
  for(size_t i = 1; i < m_params.ps(); i++) {

    std::shuffle(individual.begin(), individual.end(), RNG::instance().gen());

    pop[i].sequence = individual;
    core::recalculate_solution(m_instance, pop[i]);

  }

}


std::vector<size_t> MA::generate_random_sequence() {

  std::vector<size_t> v(m_instance.num_jobs());

  std::iota(v.begin(), v.end(), 0);

  std::shuffle(v.begin(), v.end(), RNG::instance().gen());

  return v;

}


size_t MA::selection() {

  size_t i = RNG::instance().generate((size_t) 0, pop.size()-1);
  size_t j = i;

  while(i == j) j = RNG::instance().generate((size_t) 0, pop.size()-1);

  if(pop[i].cost > pop[j].cost) {
    return j;
  } else {
    return i;
  }

}


Solution MA::path_relink_swap(const Solution &beta, const Solution &pi) {

  Solution best;
  Solution current = beta;
  size_t n = m_instance.num_jobs();

  size_t difference = 0;
  for(size_t k = 0; k < n; k++) {
    if(beta.sequence[k] != pi.sequence[k]) {
      difference++;
    }
  }

  if(difference <= 2) {
    mutation(current);
  } 

  size_t i = 0;
  for(size_t cnt = 0; cnt < n; cnt++) {

    size_t job = current.sequence[i];
    for(size_t j = 0; j < n; j++) {
      
      if(job != pi.sequence[j]) {
        continue;
      }

      if(i == j) {
        i++;
      } else {
        std::swap(current.sequence[i], current.sequence[j]);
        core::recalculate_solution(m_instance, current);

        if(current.cost < best.cost) {
          best = current;
        }
      }

      break;
    }
  }

  return best;

}


void MA::mutation(Solution &individual) {

  size_t insertion_position = RNG::instance().generate((size_t) 0, individual.sequence.size()-1);
  size_t job_position = insertion_position;

  while(insertion_position == job_position) { 
    job_position = RNG::instance().generate((size_t) 0, individual.sequence.size()-1);
  }

  individual.sequence.insert(individual.sequence.begin()+insertion_position, individual.sequence[job_position]);

  if (insertion_position > job_position) {
    individual.sequence.erase(
        individual.sequence.begin() + job_position);
  }
  else {
    individual.sequence.erase(
        individual.sequence.begin() + job_position + 1);
  }

  core::recalculate_solution(m_instance, individual);

}


bool MA::equal_solution(Solution &s1, Solution &s2) {

  if(s1.cost != s2.cost) {
    return false;
  }

  for(size_t i = 0; i < s1.sequence.size(); i++) {

    if(s1.sequence[i] != s2.sequence[i]) {
      return false;
    }

  }

  return true;

}


void MA::population_updating(std::vector<Solution> &offspring_population) {

  const size_t C = 1e7;

  for(size_t i = 0; i < offspring_population.size(); i++) {
    
    bool already_exist = false;
    size_t replaced_individual = C;
    for(size_t j = 0; j < pop.size(); j++) {

      if(equal_solution(offspring_population[i], pop[j])) {
        already_exist = true;
    
        break;
      }

      if(offspring_population[i].cost > pop[j].cost || replaced_individual != C) {
        continue;
      }

      replaced_individual = j;

    }


    if(!already_exist && replaced_individual != C) {
      pop[replaced_individual] = offspring_population[i];
    }

  }

}


void MA::restart_population() {

  auto sort_criteria = [](Solution &p1, Solution &p2) {
    return p1.cost < p2.cost;
  };

  std::sort(pop.begin(), pop.end(), sort_criteria);

  for(size_t i = 0; i < pop.size(); i++) {

    if(i < pop.size() / 2) {
      mutation(pop[i]);
      mutation(pop[i]);
    } else {
      pop[i].sequence = generate_random_sequence();
      core::recalculate_solution(m_instance, pop[i]);
    }

  }

}


Solution MA::solve() {

  Solution best_solution;
  std::vector<size_t> ref;

  generate_initial_pop(); 
  
  auto sort_criteria = [](Solution &p1, Solution &p2) {
    return p1.cost < p2.cost;
  };

  std::sort(pop.begin(), pop.end(), sort_criteria);

  best_solution = pop[0];

  ref = best_solution.sequence;
  if(rls(best_solution, ref, m_instance)) {
    core::recalculate_solution(m_instance, best_solution);
  }



  size_t count = 0;
  while(true) {

    std::vector<Solution> offspring_population;

    while(offspring_population.size() < m_params.ps()) {

      size_t parent_1 = selection();
      size_t parent_2 = parent_1;
      while(parent_2 == parent_1) parent_2 = selection();

      Solution offspring1, offspring2;
      if(RNG::instance().generate_real_number(0.0, 1.0) < m_params.pc()) {
        offspring1 = path_relink_swap(pop[parent_1], pop[parent_2]);
        offspring2 = path_relink_swap(pop[parent_2], pop[parent_1]);
      } else {
        offspring1 = pop[parent_1];
        offspring2 = pop[parent_2];
      }

      if(RNG::instance().generate_real_number(0.0, 1.0) < m_params.pm()) {
        mutation(offspring1);
      }
      if(RNG::instance().generate_real_number(0.0, 1.0) < m_params.pm()) {
        mutation(offspring2);
      }
      
      if(!equal_solution(offspring1, pop[parent_1]) && !equal_solution(offspring1, pop[parent_2])) {
        ref = offspring1.sequence;
    
        if(rls(offspring1, ref, m_instance)) {
          core::recalculate_solution(m_instance, offspring1);
        } 
    
        offspring_population.push_back(offspring1);
      }
      if(!equal_solution(offspring2, pop[parent_1]) && !equal_solution(offspring2, pop[parent_2])) {
        ref = offspring2.sequence;
    
        if(rls(offspring2, ref, m_instance)) {
          core::recalculate_solution(m_instance, offspring2);
        }
    
        offspring_population.push_back(offspring2);
      }

    }

    std::sort(offspring_population.begin(), offspring_population.end(), sort_criteria);

    population_updating(offspring_population);

    std::sort(pop.begin(), pop.end(), sort_criteria);


    count++;

    if(pop[0].cost < best_solution.cost) {
      best_solution = pop[0];
  
      count = 0;
    }

    if(count >= m_params.gamma()) {
      count = 0;
      restart_population();
    }

    if (uptime() > time_limit) {
      break;
    }

  }

  pop.clear();
  return best_solution;

}