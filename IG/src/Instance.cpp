#include "Instance.h"

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
        std::istringstream iss(current_line);

        size_t number = std::numeric_limits<size_t>::max();

        std::vector<size_t> temporary;
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
}
