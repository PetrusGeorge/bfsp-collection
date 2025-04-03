#include "hmgHS.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>

namespace {
size_t uptime() {
    static const auto global_start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - global_start_time);
    return duration.count();
}
} // namespace

hmgHS::hmgHS(Instance instance, Parameters params) : m_instance(std::move(instance)), m_params(std::move(params)) {
    this->m_time_limit = ro * m_instance.num_jobs() * m_instance.num_machines() / 2;
}

void hmgHS::generate_initial_pop() {

    m_pop = std::vector<Solution>(m_params.ms());
    Solution s;
    m_pop[0] = s;
    m_pop[1] = s;
  
    for (size_t i = 2; i < m_params.ms(); i++) {  

      s.harmony = generate_random_harmony();

      auto sort_criteria = [&s](size_t i, size_t j) { return s.harmony[i] > s.harmony[j]; };
      std::vector<size_t> phi(m_instance.num_jobs());
      std::iota(phi.begin(), phi.end(), 0);

      std::sort(phi.begin(), phi.end(), sort_criteria);
      
      s.sequence = phi;

      core::recalculate_solution(m_instance, s);

      revision(s);

      sort_permutation(s);

      m_pop[i] = s;

    }

    std::vector<size_t> phi(m_instance.num_jobs());
    std::iota(phi.begin(), phi.end(), 0);

    NEH neh(m_instance);
    m_pop[0] = neh.solve(phi);
    core::recalculate_solution(m_instance, m_pop[0]);
    m_pop[0].harmony = std::vector<double>(m_instance.num_jobs(), 0.0);
    m_pop[1].harmony = std::vector<double>(m_instance.num_jobs(), 0.0);
    permutation_to_harmony(m_pop[0]);

    NEH_WPT neh_wpt(m_instance);
    m_pop[1] = neh_wpt.solve();
    core::recalculate_solution(m_instance, m_pop[1]);
    m_pop[1].harmony = std::vector<double>(m_instance.num_jobs(), 0.0);
    permutation_to_harmony(m_pop[1]);

}

std::vector<size_t> hmgHS::generate_random_sequence() {

  std::vector<size_t> v(m_instance.num_jobs());
  std::iota(v.begin(), v.end(), 0);
  std::shuffle(v.begin(), v.end(), RNG::instance().gen());

  return v;
}

std::vector<double> hmgHS::generate_random_harmony() {
  size_t n = m_instance.num_jobs();

  std::vector<double> harmony(n);

  for(size_t j = 0; j < n; j++) {
    double r = RNG::instance().generate_real_number(0.0, 1.0);
    harmony[j] = (2 * r) - 1;
  }

  return harmony;

}

void hmgHS::permutation_to_harmony(Solution &s) {
  size_t n = m_instance.num_jobs();

  double min = std::numeric_limits<double>::max(); 
  double max = std::numeric_limits<double>::lowest();
  for(size_t i = 0; i < m_params.ms(); i++) {

    auto [s_min, s_max] = std::minmax_element(m_pop[i].harmony.begin(), m_pop[i].harmony.end());  
    if(*s_min < min) {
      min = *s_min;
    } 

    if(*s_max > max) {
      max = *s_max;
    }

  }

  double normalized_delta = (max - min) / (n - 1);

  for(size_t j = 0; j < n; j++) {
    s.harmony[ s.sequence[j] ] = max - (normalized_delta * j);
  }
 
}

void hmgHS::harmony_to_permutation(Solution &s) {
  std::vector<size_t> permutation(m_instance.num_jobs());

  std::iota(permutation.begin(), permutation.end(), 0);

  auto sort_criteria = [&s](size_t i, size_t j) { return s.harmony[i] > s.harmony[j]; };

  std::sort(permutation.begin(), permutation.end(), sort_criteria);

  s.sequence.swap(permutation);

}

std::pair<double, double> hmgHS::get_min_max_of_position(size_t j) {
  double min = std::numeric_limits<double>::max(); 
  double max = std::numeric_limits<double>::lowest();
  for(size_t i = 0; i < m_params.ms(); i++) {

    if(m_pop[i].harmony[j] > max) {
      max = m_pop[i].harmony[j];
    }

    if(m_pop[i].harmony[j] < min) {
      min = m_pop[i].harmony[j];
    }

  }

  return std::make_pair(min, max);

}

std::vector<double> hmgHS::improvise_new_harmony() {
  
  size_t n = m_instance.num_jobs();
  std::vector<double> new_harmony(n);

  for(size_t j = 0; j < n; j++) {

    double r1 = RNG::instance().generate_real_number(0.0, 1.0);
    if(r1 < m_params.pcr()) {

      size_t alpha = RNG::instance().generate((size_t) 0, m_params.ms()-1);
      new_harmony[j] = m_pop[alpha].harmony[j];

      double r2 = RNG::instance().generate_real_number(0.0, 1.0);
      if(r2 < m_params.par()) {
        new_harmony[j] = m_pop[0].harmony[j];
      }

    } else {

      auto [min, max] = get_min_max_of_position(j);

      double r3 = RNG::instance().generate_real_number(0.0, 1.0);
      new_harmony[j] = min + (max - min) * r3;

    }

  }

  return new_harmony;

}

void hmgHS::revision(Solution &s) {
  size_t n = m_instance.num_jobs();
  std::vector<double> z = s.harmony;
  std::sort(z.begin(), z.end(), [](double a, double b){ return a > b; });

  for(size_t j = 0; j < n-1; j++) {
    if(z[j] == z[j+1]) {
      if(j != 0) {
        z[j] += (z[j-1] - z[j]) / n;
      } else {
        z[j] += 0.01;
      }
    }
  }

  s.harmony.swap(z);
}

void hmgHS::sort_permutation(Solution &s) {

  size_t n = m_instance.num_jobs();

  size_t i = 0;
  for(size_t cnt = 0; cnt < n; cnt++) {

    if(i == s.sequence[i]) { 
      i++;
      continue;
    }
    size_t idx = s.sequence[i];
    std::swap(s.harmony[i], s.harmony[ idx ]);
    std::swap(s.sequence[i], s.sequence[ idx ]);

  }

}

void hmgHS::update(Solution &s) {
  size_t i = m_params.ms() - 1;
  while(true) {
    if(s.cost < m_pop[i].cost) {
      if(i == 0) {
        break;
      }
      i--;
    } else {
      i++;
      break;
    }
  }

  if(i < m_params.ms()) {
    m_pop.insert(m_pop.begin() + i, s);
    m_pop.erase(m_pop.begin() + m_params.ms());
  }

}

Solution hmgHS::solve() {

    Solution best_solution;
    std::vector<size_t> ref;

    generate_initial_pop();

    auto sort_criteria = [](Solution &s1, Solution &s2) { return s1.cost < s2.cost; };

    std::sort(m_pop.begin(), m_pop.end(), sort_criteria);

    best_solution = m_pop[0];

    while (true) {

        Solution new_solution;
        new_solution.harmony = improvise_new_harmony();

        revision(new_solution);

        harmony_to_permutation(new_solution);

        std::sort(new_solution.harmony.begin(), new_solution.harmony.end());

        ref = generate_random_sequence();
        rls(new_solution, ref, m_instance);

        core::recalculate_solution(m_instance, new_solution);

        sort_permutation(new_solution);

        update(new_solution);

        if (m_pop[0].cost < best_solution.cost) {
            best_solution = m_pop[0];
        }

        if (uptime() > m_time_limit) {
            break;
        }

    }

    m_pop.clear();
    return best_solution;
}
