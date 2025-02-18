#ifndef PW_H
#define PW_H

#include <iostream>
#include <vector>
#include <limits>
#include <numeric>
#include "Core.h"
#include "Instance.h"
#include "Solution.h"

class PW {
	public:
		PW();
		PW(const Instance &instance);
		~PW();

		std::vector<double> calculate_avg_processing_time(const size_t candidate_job, std::vector<size_t> &unscheduled);
		void update_avg_processing_time(const size_t previus_job, const size_t next_job, const size_t qt_unscheduled, std::vector<double> &artificial_processing_times);
		std::vector<double> calculate_artificial_departure_time(std::vector<std::vector<size_t>> &d, std::vector<double> &artificial_processing_times);

		// chi is basically the sigma for the artificial job
		double calculate_chi(std::vector<size_t> &new_departure_time, std::vector<double> &artificial_departure_time, std::vector<double> &artificial_processing_times); 
		double calculate_f(std::vector<std::vector<size_t>> &d, std::vector<size_t> &new_departure_time, double chi, size_t job, int k); 
		
		Solution solve();

	private:
		const Instance *instance;

};

#endif

