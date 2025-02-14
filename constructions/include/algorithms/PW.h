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

		
		std::vector<double> computeAvgProcessingTime(size_t candidate_job, std::vector<size_t> &unscheduled);
		std::vector<double> computeArtificialDepartureTime(std::vector<std::vector<size_t>> &d, std::vector<double> &artificialProcessingTimes);
		std::vector<size_t>	 computeNewDepartureTime(std::vector<std::vector<size_t>> &d, size_t node);

		// double computeWeight(double i, double k);		
		double computeChi(std::vector<size_t> &newDepartureTime, std::vector<double> &artificialDepartureTime, std::vector<double> &artificialProcessingTimes); 
		double computeSigma(std::vector<std::vector<size_t>> &d, std::vector<size_t> &newDepartureTime, size_t job, double k);
		double computeF(std::vector<std::vector<size_t>> &d, std::vector<size_t> &newDepartureTime, double chi, size_t job, int k); 
		
		Solution applyPW();

	private:
		const Instance *instance;

};

#endif

