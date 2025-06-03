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
  auto duration =
      std::chrono::duration_cast<std::chrono::seconds>(now - global_start_time);
  return static_cast<double>(duration.count());
}
} // namespace

RAIS::RAIS(Instance instance, Parameters params)
    : m_instance(std::move(instance)), m_params(std::move(params)) {
  if (auto tl = m_params.time_limit()) {
    this->m_time_limit = *tl;
  } else {
    this->m_time_limit = m_instance.num_jobs() * m_instance.num_machines() *
                         m_params.ro() / 1000;
  }

  size_t processing_times_sum = 0;
  std::vector<size_t> vec_processing_times_sum =
      m_instance.processing_times_sum();

  for (size_t i = 0; i < vec_processing_times_sum.size(); i++) {
    processing_times_sum += vec_processing_times_sum[i];
  }

  this->m_T = 0.6 * static_cast<double>(processing_times_sum) /
              (m_instance.num_jobs() * 10);

  m_departure_times = std::vector<std::vector<size_t>>(m_instance.num_jobs(), std::vector<size_t>(m_instance.num_machines(), 0));
  m_auxiliar_matrix = std::vector<std::vector<size_t>>(m_instance.num_jobs(), std::vector<size_t>(m_instance.num_machines(), 0));
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
    core::recalculate_solution(m_instance, m_pop[i], m_departure_times);
  }
}

void RAIS::clone_antibodies(std::vector<Solution> &clones) {

  size_t k = 0;
  for (size_t i = 0; i < m_params.nc(); i++) {

    core::recalculate_solution(m_instance, m_pop[i], m_departure_times);
    for (size_t j = 0; j < m_params.nc() - i; j++) {
      clones[k] = m_pop[i];
      mutation(clones[k], false);
      k++;
    }
  }

}

void RAIS::mutation(Solution &antibody, bool recalculate) {

  size_t n = m_instance.num_jobs();

  // getting indexes for movements
  size_t i = RNG::instance().generate((size_t)0, n - 2);
  size_t j = RNG::instance().generate((size_t)i + 1, n - 1);

  // Swap
  if (RNG::instance().generate_real_number(0.0, 1.0) > 0.5) {

    std::swap(antibody.sequence[i], antibody.sequence[j]);

  }
  // Insertion
  else {
    std::rotate(antibody.sequence.begin() + i,
                antibody.sequence.begin() + j,
                antibody.sequence.begin() + j + 1);
  }

  if (recalculate) {
    core::partial_recalculate_solution(m_instance, antibody, m_departure_times, m_auxiliar_matrix, 0);
  } else {
    core::partial_recalculate_solution(m_instance, antibody, m_departure_times, m_auxiliar_matrix, i);
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

  size_t i = 0;
  size_t j = m_pop.size() - 1;
  while(m_pop.size() > m_params.nc()) {

    if (nearby_antibody(m_pop[i], m_pop[j])) {
      m_pop.erase(m_pop.begin() + j);
    }

    j--;
    if (i == j) {
      i++;
      j = m_pop.size() - 1;
      if (i >= j) {
        break;
      }
    }
  }

  while(m_pop.size() > m_params.nc()) {
    m_pop.pop_back();
  }

}

void RAIS::SA() {

  for (size_t a = 0; a < m_pop.size(); a++) {

    Solution cp = m_pop[a];
    mutation(cp, true);

    if (cp.cost <= m_pop[a].cost) {
      m_pop[a] = cp;
    } else {

      size_t delta = cp.cost - m_pop[a].cost;
      if (RNG::instance().generate_real_number(0.0, 1.0) < exp(-(static_cast<double>(delta) / m_T))) {
        m_pop[a] = cp;
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
  if (m_params.benchmark()) {
    ro = {90, 60, 30};
  }

  uptime();

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

    m_pop.insert(m_pop.end(), clones.begin(), clones.end());

    std::sort(m_pop.begin(), m_pop.end(), sort_criteria);

    supression();

    if (!ro.empty() && uptime() >= (ro.back() * mxn) / 1000) {

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
