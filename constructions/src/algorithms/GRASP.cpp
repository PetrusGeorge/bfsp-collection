#include "algorithms/GRASP.h"

GRASP::GRASP() { /* ctor */ }
GRASP::GRASP(const Instance &instance) { this->instance = &instance; }

GRASP::~GRASP() { /* dtor */ }

// std::vector<size_t> GRASP::stpt_sort(const Instance &instance) {
//     std::vector<size_t> seq(instance.num_jobs());
//     std::iota(seq.begin(), seq.end(), 0);

//     std::vector<size_t> optzado;
//     optzado.reserve(instance.num_jobs());

//     for (size_t i = 0; i < instance.num_jobs(); i++) {
//         size_t sum = 0;
//         for (size_t j = 0; j < instance.num_machines(); j++) {
//             sum += instance.p(i, j);
//         }
//         optzado.push_back(sum);
//     }

//     std::sort(seq.begin(),seq.end(),
//               [optzado, instance](size_t a, size_t b) { return optzado[a] < optzado[b]; });

//     return seq;
// }


// std::vector<size_t> GRASP::computeNewDepartureTime(std::vector<std::vector<size_t>> &d, size_t node) {

// 	double m = instance->num_machines();        // number of machines

// 	auto p = core::get_reversible_matrix(*instance, false);

// 	std::vector<size_t> newDepartureTime(m);

// 	const size_t k_job = d.size() - 1;

// 	/* Calculating equal how to calculate any departure time */
// 	newDepartureTime[0] = std::max((d[ k_job ][0]) + p(node, 0), d[ k_job ][1]);

// 	for (size_t j = 1; j < m - 1; j++) {

// 		const size_t current_finish_time = newDepartureTime[j - 1] + p(node, j);

// 		newDepartureTime[j] = std::max(current_finish_time, d[ k_job ][j + 1]);

// 	}

// 	newDepartureTime.back() =
// 			newDepartureTime[m - 2] + p(node, m - 1);


// 	return newDepartureTime;

// }


// double GRASP::computeSigma(
// 	std::vector<std::vector<size_t>> &d, 
// 	std::vector<size_t> &newDepartureTime,
// 	size_t job,
// 	double k) {
	

// 	double m = instance->num_machines();        // number of machines

// 	auto p = core::get_reversible_matrix(*instance, false);

// 	double sigma = 0;

// 	for (size_t machine = 0; machine < m; machine++) {
// 		double sum;

// 		if (k == 0) sum = (newDepartureTime[machine] - p(job, machine));
// 		else sum = (newDepartureTime[machine] - d[ d.size() - 1 ][machine] - p(job, machine));

// 		sigma += sum;

// 	}


// 	return sigma;

// }


Solution GRASP::solve(double beta) {

  size_t n = instance->num_jobs(); // num jobs
  size_t m = instance->num_machines(); // num machines

  std::vector<std::vector<size_t>> d(1, std::vector<size_t>(m, 0)); // departure time matrix

  std::vector<size_t> seq = core::stpt_sort(*instance); // sorting using the stpt rule

  std::vector<size_t> pi; // sequence
  
  pi.push_back(seq[0]); // getting the job with small processing time

  std::vector<bool> bS(n, false); // vector bool to represent the jobs scheduled (true = scheduled)
  bS[ seq[0] ] = true;


  for(size_t i = 1; i < n-1; i++) {

    std::vector<size_t> RCL; // candidate list
    size_t cmin = std::numeric_limits<size_t>::max();
    size_t cmax = std::numeric_limits<size_t>::min();

    d = core::calculate_departure_times(*instance, pi, false);

    pi.push_back({});
    std::vector<size_t> sigma;
    for(size_t j = 1; j < n; j++) {
      
      sigma.push_back(0);

      if(bS[ seq[j] ]) continue;

      pi[i] = seq[j];

      // calculating the departure time of the job j that will be possibly inserted in the solution  
      std::vector<size_t> newDepartureTime = core::calculate_new_departure_time(*instance, d, pi[i]);  

      // computing the sum of the idle and blocking time of the job j
      sigma[ sigma.size()-1 ] = core::calculate_sigma(*instance, d, newDepartureTime, pi[i], i);

      // updating cmin or cmax
      if(sigma[ sigma.size()-1 ] < cmin) cmin = sigma[ sigma.size()-1 ];
      else if(sigma[ sigma.size()-1 ] > cmax) cmax = sigma[ sigma.size()-1 ];

    }

    // creating candidate list
    for(size_t j = 1; j < n; j++) {

      if(sigma[j] <= cmin + beta*(cmax - cmin)) RCL.push_back( seq[j] );

    }

    // adding job in the sequence
    size_t rand_idx = rand() % RCL.size();
    pi[ pi.size()-1 ] = RCL[rand_idx];
    bS[ RCL[rand_idx] ] = true;

  }

  // adding last remaing job in the sequence
  for(size_t i = 0; i < n; i++) {
    if(!bS[i]) {
      pi.push_back(i);
      break;
    }
  }

  // creating solution
  Solution s;
  s.sequence = pi;

  d = core::calculate_departure_times(*instance, pi, false);
	s.departure_times = d;

	s.cost = d[ n-1 ][ m-1 ];

  return s;

}