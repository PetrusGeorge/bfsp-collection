#include "constructions/GRASP.h"
#include "Core.h"

GRASP::GRASP(Instance &instance) : m_instance(instance) {}

Solution GRASP::solve(double beta, const std::vector<size_t>& initial_sequence) {
    const size_t n = m_instance.num_jobs();
    const size_t m = m_instance.num_machines();

    std::vector<std::vector<size_t>> d(1, std::vector<size_t>(m, 0));
    std::vector<size_t> seq = core::stpt_sort(m_instance);
    std::vector<size_t> pi;
    std::vector<bool> b_s(n, false);

    // Inicializa com a sequência fornecida
    if (!initial_sequence.empty()) {
        pi = initial_sequence;
        for (size_t job : pi) {
            if (job >= n) throw std::runtime_error("Job inválido na sequência inicial.");
            b_s[job] = true;
        }
    } else {
        pi.push_back(seq[0]);
        b_s[seq[0]] = true;
    }

    for (size_t i = pi.size(); i < n; i++) { // Continua a partir do tamanho atual de pi
        d = core::calculate_departure_times(m_instance, pi);
        pi.resize(i + 1);

        // Coleta jobs não agendados
        std::vector<size_t> candidates;
        for (size_t job : seq) {
            if (!b_s[job]) candidates.push_back(job);
        }

        if (candidates.empty()) break;

        // Cálculo de sigma e RCL
        std::vector<size_t> sigma_values;
        size_t cmin = std::numeric_limits<size_t>::max();
        size_t cmax = 0;
        for (size_t job : candidates) {
            pi[i] = job;
            auto new_d = core::calculate_new_departure_time(m_instance, d, job);
            size_t sigma = core::calculate_sigma(m_instance, d, new_d, job, i);
            sigma_values.push_back(sigma);
            cmin = std::min(cmin, sigma);
            cmax = std::max(cmax, sigma);
        }

        // Construção da RCL
        std::vector<size_t> rcl;
        for (size_t k = 0; k < candidates.size(); k++) {
            if (sigma_values[k] <= cmin + beta * (cmax - cmin)) {
                rcl.push_back(candidates[k]);
            }
        }

        // Seleção aleatória
        if (!rcl.empty()) {
            size_t selected = rcl[rand() % rcl.size()];
            pi[i] = selected;
            b_s[selected] = true;
        } else {
            pi[i] = candidates[0];
            b_s[candidates[0]] = true;
        }
    }

    // Adiciona último job restante, se houver
    for (size_t job : seq) {
        if (!b_s[job]) {
            pi.push_back(job);
            break;
        }
    }

    Solution s;
    s.sequence = pi;
    s.departure_times = core::calculate_departure_times(m_instance, pi);
    s.cost = s.departure_times.back().back();

    return s;
}