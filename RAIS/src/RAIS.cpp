#include "RAIS.h"

#include <chrono>
#include <iostream>
#include <algorithm>
#include <limits>
#include <vector>

namespace {
double uptime() {
  static const auto global_start_time = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - global_start_time);
  return static_cast<double>(duration.count());
}
} 

RAIS::RAIS(Instance instance, Parameters params)
    : m_instance(std::move(instance)), m_params(std::move(params)) {
  if (auto tl = params.time_limit()) {
    this->time_limit = *tl;
  } else {
    this->time_limit = m_instance.num_jobs() * m_instance.num_machines() * 100;
  }
}

double RAIS::affinity_calculation(size_t cost) {
  return (1 / static_cast<double>(cost));
}

void RAIS::pop_affinity_calculation(
    std::vector<Solution> &pop) {

  for (size_t i = 0; i < pop.size(); i++) {
    pop[i].affinity = affinity_calculation(pop[i].cost);
  }

}

std::vector<Solution> RAIS::initialization() {

  size_t n = m_instance.num_jobs();
  size_t npop = (m_params.nc() * (m_params.nc() + 1) / 2);

  std::vector<Solution> init_pop(npop);

  std::vector<size_t> antibody(n);
  std::iota(antibody.begin(), antibody.end(), 0);

  for (size_t i = 0; i < npop; i++) {

    std::shuffle(antibody.begin(), antibody.end(), RNG::instance().gen());

    init_pop[i].sequence = antibody;
    core::recalculate_solution(m_instance, init_pop[i]);
    init_pop[i].affinity = affinity_calculation(init_pop[i].cost);
  }

  return init_pop;
}

std::vector<Solution>
RAIS::clone_antibodies(const std::vector<Solution> &pop) {

  std::vector<Solution> clones(m_params.nc() * (m_params.nc() + 1) / 2);
  size_t k = 0;
  for (size_t i = 0; i < m_params.nc(); i++) {

    for (size_t j = 0; j < m_params.nc() - i; j++) {
      clones[k] = pop[i];
      k++;
    }
  }

  return clones;
}

void RAIS::mutation(std::vector<Solution> &pop) {

  size_t n = m_instance.num_jobs();
  double p;

  for (size_t antibody = 0; antibody < pop.size(); antibody++) {

    // getting indexes for movements
    size_t i = RNG::instance().generate((size_t) 0, n-2);
    size_t j = RNG::instance().generate((size_t) i+1, n-1);

    // choosing a movement
    p = RNG::instance().generate_real_number(0.0, 1.0);

    // Swap
    if (p >= 0.5) {

      std::swap(pop[antibody].sequence[i],
                pop[antibody].sequence[j]);

    }
    // Insertion
    else {
      std::rotate(pop[antibody].sequence.begin()+i, pop[antibody].sequence.begin()+j, pop[antibody].sequence.begin()+j+1);
    }

    core::recalculate_solution(m_instance, pop[antibody]);
  }
}

bool RAIS::nearby_antibody(Solution &s1, Solution &s2) {

  size_t d = 0;
  for (size_t i = 0; i < s1.sequence.size(); i++) {

    if (s1.sequence[i] == s2.sequence[i]) {
      d++;
      if (d >= m_params.d_threshold()) {
        return true;
      }
    }

  }

  return false;
}

void RAIS::supression(std::vector<Solution> &pop) {
  

  std::vector<bool> eliminated(pop.size(), false);
  for (size_t i = 0; i < pop.size(); i++) {
    
    if (eliminated[i]) {
      continue;
    }

    for (size_t j = i+1; j < pop.size(); j++) {
      
      if (!nearby_antibody(pop[i], pop[j])) {
        continue;
      }

      eliminated[j] = true;
    }
  }

  for (size_t i = eliminated.size()-1; i < eliminated.size(); i--) {
    if (eliminated[i]) {
      pop.erase(pop.begin()+i);
      
      if (pop.size() == m_params.nc()) {
        return;
      }
    }
  }

  while(pop.size() > m_params.nc()) {
    pop.pop_back();
  }

}

void RAIS::SA(std::vector<Solution> &pop, double T) {

  std::vector<Solution> cp_pop = pop;
  mutation(cp_pop);

  for (size_t a = 0; a < pop.size(); a++) {

    if (cp_pop[a].cost <= pop[a].cost) {
      pop[a] = cp_pop[a];
      pop[a].affinity = affinity_calculation(pop[a].cost);
    } else {

      size_t delta = cp_pop[a].cost - pop[a].cost;
      double r = RNG::instance().generate_real_number(0.0, 1.0);
      double p = 1 / exp((static_cast<double>(delta) / T));

      if (r < p) {
        pop[a] = cp_pop[a];
        pop[a].affinity = affinity_calculation(pop[a].cost);
      }

    }
  }

}

void RAIS::select_nc_best(std::vector<Solution> &pop) {
  std::vector<Solution> aux(m_params.nc());

  for(size_t i = 0; i < m_params.nc(); i++) {
    
    size_t best_idx = i;
    for(size_t j = i+1; j < pop.size(); j++) {
      if(pop[j].affinity > pop[best_idx].affinity) {
        best_idx = j;
      }
    }

    aux[i] = pop[best_idx];
    std::swap(pop[best_idx], pop[i]);
  }

  pop.swap(aux);

}

Solution RAIS::solve() {

  std::cout << "Tempo de parada: " << time_limit << " us" << std::endl;

  size_t n = m_instance.num_jobs();

  size_t G = 1;

  size_t processing_times_sum = 0;
  std::vector<size_t> vec_processing_times_sum =
      m_instance.processing_times_sum();

  for (size_t i = 0; i < vec_processing_times_sum.size(); i++) {
    processing_times_sum += vec_processing_times_sum[i];
  }

  // initial tempeture
  double T = 0.6 * static_cast<double>(processing_times_sum) /
             (n * 10); 

  Solution best_solution;
  std::vector<Solution> pop;

  pop = initialization();

  auto sort_criteria = [](Solution &p1,
                          Solution &p2) {
    return p1.affinity > p2.affinity;
  };

  // double timer_counter = 1;
  while (true) {
    select_nc_best(pop);

    std::vector<Solution> clones =
        clone_antibodies(pop); // cloning the best antibodies

    mutation(clones);

    pop_affinity_calculation(clones);

    pop.insert(pop.end(), clones.begin(), clones.end());

    std::sort(pop.begin(), pop.end(), sort_criteria);

    supression(pop);

    if (pop[0].cost < best_solution.cost) {
      best_solution = pop[0];
    }

    SA(pop, T);

    if (uptime() > time_limit) {
      break;
    }
    // else if (uptime() > timer_counter * time_limit / 5) {
    //     std::cout << timer_counter * time_limit / 5 << " Microsseconds out of
    //     " << time_limit << '\n'; timer_counter++;
    // }

    G++;

    if (G >= m_params.Gt()) {
      T *= m_params.alpha();
      G = 1;
    }
  }

  return best_solution;
}
