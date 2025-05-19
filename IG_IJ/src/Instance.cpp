#include "Instance.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

Instance::Instance(const std::filesystem::path &path) {

    std::ifstream file(path);

    if (!file) {
        throw std::runtime_error("Could not read file");
    }

    std::string current_line;
    getline(file, current_line);
    m_num_jobs = std::stoull(current_line);

    getline(file, current_line);
    m_num_machines = std::stoull(current_line);

    m_matrix.reserve(m_num_jobs);
    while (getline(file, current_line)) {

        if (current_line.size() <= 1) {
            continue;
        }

        std::istringstream iss(current_line);

        size_t number = std::numeric_limits<size_t>::max();

        std::vector<long> temporary;
        temporary.reserve(m_num_machines);

        while (iss >> number) {
            temporary.emplace_back(number);
        }

        if (temporary.size() != m_num_machines) {
            throw std::runtime_error("Wrong number of machines on instance");
        }
        m_matrix.emplace_back(std::move(temporary));
    }

    if (m_matrix.size() != m_num_jobs || !file.eof()) {
        throw std::runtime_error("Wrong number of jobs on instance");
    }

    // Used for LPT
    calculate_processing_times_sum();
    calculate_all_processing_times_sum();
}

void Instance::calculate_processing_times_sum() {
    m_processing_times_sum.reserve(m_num_jobs);
    for (size_t i = 0; i < m_num_jobs; i++) {
        size_t sum = 0;
        for (size_t j = 0; j < m_num_machines; j++) {
            sum += m_matrix[i][j];
        }
        m_processing_times_sum.push_back(sum);
    }
}

Instance Instance::create_reverse_instance() {
    Instance reverse = *this;

    for (auto &line : reverse.m_matrix) {
        std::reverse(line.begin(), line.end());
    }

    return reverse;
}

void Instance::calculate_all_processing_times_sum(){
    for(size_t i =0; i < m_num_jobs; i++) {
        m_all_processing_times_sum += m_processing_times_sum[i];
    }
}