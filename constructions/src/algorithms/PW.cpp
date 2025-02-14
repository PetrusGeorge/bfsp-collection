#include "algorithms/PW.h"

PW::PW() { /*ctor */ }
PW::~PW() { /* dtor */ }

PW::PW(const Instance &instance) {
	this->instance = &instance;
}


std::vector<double> PW::computeAvgProcessingTime(size_t candidate_job, std::vector<size_t> &unscheduled) {

	double m = instance->num_machines();        // number of machines
	
	std::vector<double> artificialTimes(m, 0); // m processing times


	auto p = core::get_reversible_matrix(*instance, false);

	for (size_t j = 0; j < unscheduled.size(); j++) {
	
		size_t job = unscheduled[j];
	
		if(job == candidate_job) continue; // avoiding the candidate job
		
		// getting the sum of the processing time on each machine for each job
		for(size_t machine = 0; machine < m; machine++) artificialTimes[machine] += p(job, machine);
		
	}
	
	// dividing each processing time by (n - k - 1)
	for (auto i = 0; i < m; i++) {
		artificialTimes[i] /= ((double) unscheduled.size() - 1);
	}	

	return artificialTimes;

}


std::vector<double> PW::computeArtificialDepartureTime(
	std::vector<std::vector<size_t>> &d, 
	std::vector<double> &artificialProcessingTimes
	) {

	double m = instance->num_machines();        // number of machines
	auto p = core::get_reversible_matrix(*instance, false);

	std::vector<double> artificialDepartureTime(m);

	artificialDepartureTime[0] = std::max(
		((double) d[ d.size() - 1 ][0]) + artificialProcessingTimes[0], (double) d[ d.size() - 1 ][1]
	);

	for (size_t j = 1; j < m - 1; j++) {

			const double current_finish_time = artificialDepartureTime[j - 1] + artificialProcessingTimes[j];

			artificialDepartureTime[j] = std::max(current_finish_time, (double)  d[ d.size() - 1 ][j + 1]);
	}

	artificialDepartureTime.back() =
		artificialDepartureTime[m - 2] + artificialProcessingTimes[m - 1];


	return artificialDepartureTime;

}


std::vector<size_t> PW::computeNewDepartureTime(std::vector<std::vector<size_t>> &d, size_t node) {

	double m = instance->num_machines();        // number of machines

	auto p = core::get_reversible_matrix(*instance, false);

	std::vector<size_t> newDepartureTime(m);

	const size_t k_job = d.size() - 1;

	/* Calculating equal how to calculate any departure time */
	newDepartureTime[0] = std::max((d[ k_job ][0]) + p(node, 0), d[ k_job ][1]);

	for (size_t j = 1; j < m - 1; j++) {

		const size_t current_finish_time = newDepartureTime[j - 1] + p(node, j);

		newDepartureTime[j] = std::max(current_finish_time, d[ k_job ][j + 1]);

	}

	newDepartureTime.back() =
			newDepartureTime[m - 2] + p(node, m - 1);


	return newDepartureTime;

}


double PW::computeChi(
	std::vector<size_t> &newDepartureTime, 
	std::vector<double> &artificialDepartureTime, 
	std::vector<double> &artificialProcessingTimes
	) {
	
	double m = instance->num_machines();        // number of machines

	double chi = 0;
	
	for (size_t machine = 0; machine < m; machine++) {
	
		double sum = (artificialDepartureTime[machine] - newDepartureTime[machine] - artificialProcessingTimes[machine]);

		chi += sum;

	}

	return chi;	

}


double PW::computeSigma(
	std::vector<std::vector<size_t>> &d, 
	std::vector<size_t> &newDepartureTime,
	size_t job,
	double k) {
	

	double m = instance->num_machines();        // number of machines

	auto p = core::get_reversible_matrix(*instance, false);

	double sigma = 0;

	for (size_t machine = 0; machine < m; machine++) {
		double sum;

		if (k == 0) sum = (newDepartureTime[machine] - p(job, machine));
		else sum = (newDepartureTime[machine] - d[ d.size() - 1 ][machine] - p(job, machine));

		sigma += sum;

	}


	return sigma;

}


double PW::computeF(
	std::vector<std::vector<size_t>> &d, 
	std::vector<size_t> &newDepartureTime, 
	double chi, 
	size_t job, 
	int k
	) {

	double n = instance->num_jobs();         // number of jobs
	double sigma = computeSigma(d, newDepartureTime, job, k);

	double f = ( (n - k - 2) * sigma + chi );

	return f;	

}


Solution PW::applyPW() {

	size_t n = instance->num_jobs();         // number of jobs
	size_t m = instance->num_machines();     // number of machines

	std::vector<size_t> newSeq; // partial sequence 
	std::vector<size_t> unscheduled(n); // list of unscheduled jobs (initially 0, 1, ..., n)
	std::iota(unscheduled.begin(), unscheduled.end(), 0);

	std::vector<double> artificialProcessingTimes; // processing time of the artificial job v 
	std::vector<double> artificialDepartureTime; // departure time of the artificial job v 
	std::vector<size_t> newDepartureTime; // hipotetical departure time of the job j 

	size_t best_i = std::numeric_limits<size_t>::max(); // store the index of the variable with smallest f
	double smallestF = std::numeric_limits<double>::max(); // smallest f
	double smallestChi = std::numeric_limits<double>::max(); // smallest chi

	double chi;
	double f;

	std::vector<std::vector<size_t>> d_current; // current departure time 

//////////////////////////// allocating the first job
/* revisar isso, talvez possa ser colocado dentro do loop */

	newSeq.push_back({});
	for(size_t i = 0; i < n; i++) {
		newSeq[0] = i;

		d_current = core::calculate_departure_times(*instance, newSeq, false);

		artificialProcessingTimes = computeAvgProcessingTime(i, unscheduled);

		artificialDepartureTime = computeArtificialDepartureTime(d_current, artificialProcessingTimes);

		chi = computeChi(d_current[0], artificialDepartureTime, artificialProcessingTimes);

		f = computeF(d_current, d_current[0], chi, i, 0);

		if(f < smallestF || (f == smallestF && chi < smallestChi)) {
			best_i = i;
			smallestChi = chi;
			smallestF = f;
		} 			

	}

	newSeq[0] = best_i;
	unscheduled.erase(unscheduled.begin() + best_i);

////////////////////////////////////////////////////


	for(size_t k = 1; k <= n-2; k++) {

		d_current = core::calculate_departure_times(*instance, newSeq, false);

		best_i = std::numeric_limits<size_t>::max();
		smallestChi = std::numeric_limits<size_t>::max();
		smallestF = std::numeric_limits<size_t>::max();

		for(size_t i = 0; i < unscheduled.size(); i++) {

			artificialProcessingTimes = computeAvgProcessingTime(unscheduled[i], unscheduled);
			artificialDepartureTime = computeArtificialDepartureTime(d_current, artificialProcessingTimes);
			newDepartureTime = computeNewDepartureTime(d_current, unscheduled[i]);

			chi = computeChi(newDepartureTime, artificialDepartureTime, artificialProcessingTimes);
			f = computeF(d_current, newDepartureTime, chi, unscheduled[i], k);

			if(f < smallestF || (f == smallestF && chi < smallestChi)) {
				best_i = i;
				smallestChi = chi;
				smallestF = f;
			} 			

		}

		newSeq.push_back(unscheduled[best_i]);
		unscheduled.erase(unscheduled.begin() + best_i);

	}

	newSeq.push_back(unscheduled[0]);

	Solution s;
	s.sequence = newSeq;
	
	d_current = core::calculate_departure_times(*instance, newSeq, false);
	s.departure_times = d_current;

	s.cost = d_current[ n-1 ][ m-1 ];

	return s;

}

