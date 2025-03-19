#include "RAIS.h"

#include <chrono>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <limits>
#include <vector>

namespace {
double uptime() {
  static const auto global_start_time = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      now - global_start_time);
  return static_cast<double>(duration.count());
}
} // namespace

RAIS::RAIS(Instance instance, Parameters params)
    : m_instance(std::move(instance)), m_params(std::move(params)) {
  this->time_limit = instance.num_jobs() * instance.num_machines() * 100;
}

double RAIS::affinity_calculation(size_t cost) {
  return (1 / static_cast<double>(cost));
}

void RAIS::pop_affinity_calculation(
    std::vector<std::pair<Solution, double>> &pop) {

  for (size_t i = 0; i < pop.size(); i++)
    pop[i].second = affinity_calculation(pop[i].first.cost);
}

std::vector<std::pair<Solution, double>> RAIS::initial_pop() {

  size_t n = m_instance.num_jobs();
  size_t npop = (m_params.nc() * (m_params.nc() + 1) / 2);

  std::vector<std::pair<Solution, double>> init_pop(npop);

  std::vector<size_t> antibody(n);
  std::iota(antibody.begin(), antibody.end(), 0);

  for (size_t i = 0; i < npop; i++) {

    std::shuffle(antibody.begin(), antibody.end(), RNG::instance().gen());

    init_pop[i].first.sequence = antibody;
    core::recalculate_solution(m_instance, init_pop[i].first);
  }

  pop_affinity_calculation(init_pop);

  return init_pop;
}

std::vector<std::pair<Solution, double>>
RAIS::clone_antibodies(std::vector<std::pair<Solution, double>> &pop) {

  std::vector<std::pair<Solution, double>> clones(m_params.nc() * (m_params.nc() + 1) / 2);
  size_t k = 0;
  for (size_t i = 0; i < m_params.nc(); i++) {

    for (size_t j = 0; j < m_params.nc() - i; j++) {
      clones[k].first.sequence = pop[i].first.sequence;
      k++;
    }
  }

  return clones;
}

void RAIS::mutation(std::vector<std::pair<Solution, double>> &pop) {

  size_t n = m_instance.num_jobs();
  double p;

  for (size_t antibody = 0; antibody < pop.size(); antibody++) {

    // getting indexes for movements
    size_t i = RNG::instance().generate((size_t) 0, n-1);
    size_t j = i;

    while (i == j)
      j = RNG::instance().generate((size_t) 0, n-1);

    // choosing a movement
    p = RNG::instance().generateDouble();

    // Swap
    if (p >= 0.5) {

      std::swap(pop[antibody].first.sequence[i],
                pop[antibody].first.sequence[j]);

    }
    // Insertion
    else if (p < 0.5) {

      pop[antibody].first.sequence.insert(pop[antibody].first.sequence.begin() +
                                              i,
                                          pop[antibody].first.sequence[j]);

      if (i > j)
        pop[antibody].first.sequence.erase(
            pop[antibody].first.sequence.begin() + j);
      else
        pop[antibody].first.sequence.erase(
            pop[antibody].first.sequence.begin() + j + 1);
    }

    core::recalculate_solution(m_instance, pop[antibody].first);
  }
}

bool RAIS::nearby_antibody(Solution &s1, Solution &s2) {

  size_t d = 0;
  for (size_t i = 0; i < s1.sequence.size(); i++) {
    if (s1.sequence[i] == s2.sequence[i]) {
      d++;
      if (d >= m_params.d_threshold())
        return false;
    }
  }

  return true;
}

void RAIS::supression(std::vector<std::pair<Solution, double>> &pop,
                      std::vector<std::pair<Solution, double>> &clones) {

  std::vector<bool> pop_eliminated(pop.size(), false);
  std::vector<bool> clone_eliminated(clones.size(), false);
  for (size_t i = 0; i < pop.size(); i++) {

    for (size_t j = 0; j < clones.size(); j++) {

      if (clone_eliminated[j])
        continue;

      if (nearby_antibody(pop[i].first, clones[j].first)) {

        if (pop[i].second > clones[j].second) {
          clone_eliminated[j] = true;
        } else {
          pop_eliminated[i] = true;
          break;
        }
      }
    }
  }

  // taking the nc remaining antibodies
  std::vector<std::pair<Solution, double>> aux(m_params.nc());

  size_t i = 0;
  size_t j = 0;
  for (size_t k = 0; k < m_params.nc(); k++) {

    while (i < pop.size() && pop_eliminated[i])
      i++;

    while (j < pop.size() && clone_eliminated[j])
      j++;

    if (i >= pop.size()) {
      aux[k] = clones[j];
    } else if (j >= clones.size()) {
      aux[k] = pop[i];
    } else if (pop[i].second > clones[j].second) {
      aux[k] = pop[i];
    } else {
      aux[k] = clones[j];
    }
  }

  pop.swap(aux);
}

void RAIS::update(std::vector<std::pair<Solution, double>> &pop,
                  std::vector<std::pair<Solution, double>> &clones) {

  std::vector<std::pair<Solution, double>> aux(m_params.nc());
  size_t i = 0, j = 0;

  for (size_t k = 0; k < m_params.nc(); k++) {

    if (pop[i].second > clones[j].second) {
      aux[k] = pop[i];
      i++;
    } else {
      aux[k] = clones[j];
      j++;
    }
  }

  pop.swap(aux);
}

void RAIS::SA(std::vector<std::pair<Solution, double>> &pop, double T) {

  std::vector<std::pair<Solution, double>> cp_pop = pop;
  mutation(cp_pop);

  for (size_t a = 0; a < pop.size(); a++) {

    if (cp_pop[a].first.cost <= pop[a].first.cost)
      pop[a] = cp_pop[a];

    else {
      size_t delta = cp_pop[a].first.cost - pop[a].first.cost;
      double r = RNG::instance().generateDouble();
      double p = exp(1 / (static_cast<double>(delta) * T));
      if (r < p)
        pop[a] = cp_pop[a];
    }
  }
}

Solution RAIS::solve() {

  std::cout << "Tempo de parada: " << time_limit << " us" << std::endl;
  srand(time(0));

  size_t n = m_instance.num_jobs();

  size_t G = 1;

  size_t processing_times_sum = 0;
  std::vector<size_t> vec_processing_times_sum =
      m_instance.processing_times_sum();
  for (size_t i = 0; i < vec_processing_times_sum.size(); i++)
    processing_times_sum += vec_processing_times_sum[i];
  double T = 0.6 * static_cast<double>(processing_times_sum) /
             (n * 10); // initial tempeture

  // std::cout << "processing_times_sum: " << processing_times_sum << "; T: " <<
  // T << std::endl;

  Solution best_solution;
  std::vector<std::pair<Solution, double>> pop;

  pop = initial_pop();

  auto sort_criteria = [](std::pair<Solution, double> &p1,
                          std::pair<Solution, double> &p2) {
    return p1.second > p2.second;
  };

  std::sort(pop.begin(), pop.end(),
            sort_criteria); // sorting by affinity (probably exist some best way
                            // to do this)
  pop.resize(m_params.nc());           // select

  // double timer_counter = 1;
  while (true) {

    std::vector<std::pair<Solution, double>> clones =
        clone_antibodies(pop); // cloning the best antibodies

    mutation(clones);

    pop_affinity_calculation(clones);

    std::sort(clones.begin(), clones.end(), sort_criteria);

    supression(pop, clones);

    update(pop, clones);

    if (pop[0].first.cost < best_solution.cost) {
      best_solution = pop[0].first;
      // std::cout << best_solution << std::endl;
    }

    if (uptime() > time_limit)
      break;
    // else if (uptime() > timer_counter * time_limit / 5) {
    //     std::cout << timer_counter * time_limit / 5 << " Microsseconds out of
    //     " << time_limit << '\n'; timer_counter++;
    // }

    SA(pop, T);

    std::sort(pop.begin(), pop.end(), sort_criteria);

    G++;

    if (G == m_params.Gt()) {
      T *= m_params.alpha();
      G = 0;
    }
  }

  return best_solution;
}
