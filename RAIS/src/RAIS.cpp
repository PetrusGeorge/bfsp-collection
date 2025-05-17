#include "RAIS.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <vector>

namespace {
double uptime() {
  static const auto global_start_time = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(
      now - global_start_time);
  return static_cast<double>(duration.count());
}
} // namespace

RAIS::RAIS(Instance instance, Parameters params)
    : m_instance(std::move(instance)), m_params(std::move(params)) {
  if (auto tl = params.time_limit()) {
    this->m_time_limit = *tl;
  } else {
    this->m_time_limit =
        m_instance.num_jobs() * m_instance.num_machines() * m_params.ro() / 1000;
  }

  size_t processing_times_sum = 0;
  std::vector<size_t> vec_processing_times_sum =
      m_instance.processing_times_sum();

  for (size_t i = 0; i < vec_processing_times_sum.size(); i++) {
    processing_times_sum += vec_processing_times_sum[i];
  }

  this->m_T = 0.6 * static_cast<double>(processing_times_sum) / (m_instance.num_jobs() * 10);
}

double RAIS::affinity_calculation(size_t cost) {
  return (1 / static_cast<double>(cost));
}

void RAIS::pop_affinity_calculation(std::vector<Solution> &set) {

  for (size_t i = 0; i < set.size(); i++) {
    set[i].affinity = affinity_calculation(set[i].cost);
  }
}

void RAIS::initialization() {

  size_t n = m_instance.num_jobs();
  size_t npop = (m_params.nc() * (m_params.nc() + 1) / 2);
  m_pop = std::vector<Solution>(npop);

  std::vector<size_t> antibody(n);
  std::iota(antibody.begin(), antibody.end(), 0);

  for (size_t i = 0; i < npop; i++) {

    std::shuffle(antibody.begin(), antibody.end(), RNG::instance().gen());

    m_pop[i].sequence = antibody;
    core::recalculate_solution(m_instance, m_pop[i]);
    m_pop[i].affinity = affinity_calculation(m_pop[i].cost);
  }

}

std::vector<Solution> RAIS::clone_antibodies(std::vector<Solution> &clones) {

  size_t k = 0;
  for (size_t i = 0; i < m_params.nc(); i++) {

    for (size_t j = 0; j < m_params.nc() - i; j++) {
      clones[k] = m_pop[i];
      k++;
    }
  }

  return clones;
}

void RAIS::mutation(std::vector<Solution> &set) {

  size_t n = m_instance.num_jobs();
  double p;

  for (size_t antibody = 0; antibody < set.size(); antibody++) {

    // getting indexes for movements
    size_t i = RNG::instance().generate((size_t)0, n - 2);
    size_t j = RNG::instance().generate((size_t)i + 1, n - 1);

    // choosing a movement
    p = RNG::instance().generate_real_number(0.0, 1.0);

    // Swap
    if (p >= 0.5) {

      std::swap(set[antibody].sequence[i], set[antibody].sequence[j]);

    }
    // Insertion
    else {
      std::rotate(set[antibody].sequence.begin() + i,
                  set[antibody].sequence.begin() + j,
                  set[antibody].sequence.begin() + j + 1);
    }

    core::recalculate_solution(m_instance, set[antibody]);
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

void RAIS::supression() {

  std::vector<bool> eliminated(m_pop.size(), false);
  for (size_t i = 0; i < m_pop.size(); i++) {

    if (eliminated[i]) {
      continue;
    }

    for (size_t j = i + 1; j < m_pop.size(); j++) {

      if (!nearby_antibody(m_pop[i], m_pop[j])) {
        continue;
      }

      eliminated[j] = true;
    }
  }

  for (size_t i = eliminated.size() - 1; i < eliminated.size(); i--) {
    if (eliminated[i]) {
      m_pop.erase(m_pop.begin() + i);

      if (m_pop.size() == m_params.nc()) {
        return;
      }
    }
  }

  while (m_pop.size() > m_params.nc()) {
    m_pop.pop_back();
  }
}

void RAIS::SA() {

  std::vector<Solution> cp_pop = m_pop;
  mutation(cp_pop);

  for (size_t a = 0; a < m_pop.size(); a++) {

    if (cp_pop[a].cost <= m_pop[a].cost) {
      m_pop[a] = cp_pop[a];
      m_pop[a].affinity = affinity_calculation(m_pop[a].cost);
    } else {

      size_t delta = cp_pop[a].cost - m_pop[a].cost;
      double r = RNG::instance().generate_real_number(0.0, 1.0);
      double p = 1 / exp((static_cast<double>(delta) / m_T));

      if (r < p) {
        m_pop[a] = cp_pop[a];
        m_pop[a].affinity = affinity_calculation(m_pop[a].cost);
      }
    }
  }
}

void RAIS::select_nc_best() {
  std::vector<Solution> aux(m_params.nc());

  for (size_t i = 0; i < m_params.nc(); i++) {

    size_t best_idx = i;
    for (size_t j = i + 1; j < m_pop.size(); j++) {
      if (m_pop[j].affinity > m_pop[best_idx].affinity) {
        best_idx = j;
      }
    }

    aux[i] = m_pop[best_idx];
    std::swap(m_pop[best_idx], m_pop[i]);
  }

  m_pop.swap(aux);
}

Solution RAIS::solve() {

  size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
  std::vector<size_t> ro;
  if (m_params.benchmark()){
    ro = {90, 60, 30};
  }

  std::vector<Solution> clones(m_params.nc() * (m_params.nc() + 1) / 2);
  
  size_t G = 1;

  Solution best_solution;

  initialization();

  auto sort_criteria = [](Solution &p1, Solution &p2) {
    return p1.affinity > p2.affinity;
  };

  select_nc_best();

  while (true) {

    clone_antibodies(clones); // cloning the best antibodies

    mutation(clones);

    pop_affinity_calculation(clones);

    m_pop.insert(m_pop.end(), clones.begin(), clones.end());

    std::sort(m_pop.begin(), m_pop.end(), sort_criteria);

    supression();

    if (!ro.empty() && uptime() >= (ro.back()*mxn) / 1000){
           
      std::cout << best_solution.cost << '\n';
      ro.pop_back();
    }

    if (uptime() > m_time_limit) {
      break;
    }

    if (m_pop[0].cost < best_solution.cost) {
      best_solution = m_pop[0];
    }

    SA();

    std::sort(m_pop.begin(), m_pop.end(), sort_criteria);

    G++;

    if (G == m_params.Gt()) {
      m_T *= m_params.alpha();
      G = 1;
    }
  }

  return best_solution;
}
