#include "algorithms/PF.h"
#include "Core.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <vector>

std::vector<size_t> PF::stpt_sort(const Instance &instance) {
    std::vector<size_t> seq(instance.num_jobs());
    std::iota(seq.begin(), seq.end(), 0);

    std::vector<size_t> optzado;
    optzado.reserve(instance.num_jobs());

    for (size_t i = 0; i < instance.num_jobs(); i++) {
        size_t sum = 0;
        for (size_t j = 0; j < instance.num_machines(); j++) {
            sum += instance.p(i, j);
        }
        optzado.push_back(sum);
    }

    std::sort(seq.begin(),seq.end(),
              [optzado, instance](size_t a, size_t b) { return optzado[a] < optzado[b]; });

    return seq;
}

//---------------------------------------------------------
// PF heuristic implementation.
// This function builds a new job sequence by selecting, at each iteration,
// the candidate job (among unscheduled jobs) that minimizes the sum of idle and blocking times.
// (Here we use a measure sigma computed from the difference in departure times.)
Solution PF::solve(const Instance &instance) {
    size_t n = instance.num_jobs();     // número de jobs
    size_t m = instance.num_machines(); // número de máquinas

    // Passo 1: Ordena os jobs usando a regra STPT.
    // if (sol.sequence.empty()) {
    //     PF::stpt_sort(sol, instance);
    // }

    std::vector<size_t> stpt = stpt_sort(instance);

    // Vetores auxiliares para indicar os jobs já agendados e armazenar a nova sequência.
    std::vector<bool> scheduled(n, false);
    std::vector<size_t> new_seq;
    std::vector<size_t> unscheduled;

    // Passos 2 e 3: Seleciona o primeiro job (o primeiro na ordem STPT).
    size_t first_job = stpt.front();
    new_seq.push_back(first_job);
    scheduled[first_job] = true;

    // Preenche o conjunto de jobs não agendados.
    for (size_t i = 0; i < n; ++i) {
        if (!scheduled[i]) {
            unscheduled.push_back(i);
        }
    }

    // Para cada posição na sequência (exceto a primeira), escolhe o job que minimiza a medida sigma.
    for (size_t i = 1; i < n; ++i) {
        // Calcula os departure times da sequência atual.
        std::vector<std::vector<size_t>> d_current = core::calculate_departure_times(instance, new_seq);
        size_t best_sigma = std::numeric_limits<size_t>::max();
        size_t best_job = unscheduled.front(); // valor inicial

        // Para cada job candidato não agendado:
        for (size_t candidate : unscheduled) {
            std::vector<size_t> candidate_sequence = new_seq;
            candidate_sequence.push_back(candidate);

            // Calcula os departure times da sequência candidata.
            std::vector<std::vector<size_t>> d_candidate = core::calculate_departure_times(instance, candidate_sequence);
            const std::vector<size_t> &d_new = d_candidate.back();

            // Calcula sigma como a soma, para cada máquina k (de 1 a m), de:
            //    d(candidate, k) - (d(current_last, k) + p(candidate, k))
            size_t sigma = 0;
            for (size_t k = 1; k <= m; ++k) {
                sigma += d_new[k] - (d_current.back()[k] + instance.p(candidate, k - 1));
            }
            if (sigma < best_sigma) {
                best_sigma = sigma;
                best_job = candidate;
            }
        } // fim do loop para candidatos

        // Adiciona o job selecionado à nova sequência.
        new_seq.push_back(best_job);
        scheduled[best_job] = true;
        unscheduled.erase(std::remove(unscheduled.begin(), unscheduled.end(), best_job), unscheduled.end());
    } // fim do loop principal

    Solution sol;
    // Atualiza a solução: sequência, departure_times e custo (makespan)
    sol.sequence = new_seq;
    std::vector<std::vector<size_t>> d_final = core::calculate_departure_times(instance, new_seq);
    sol.departure_times = d_final;
    sol.cost = d_final.back()[m]; // o makespan é o departure time do último job na última máquina

    return sol;
}
