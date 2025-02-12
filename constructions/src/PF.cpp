#include "PF.h"
#include <cassert>
#include <iostream>
#include <numeric>

PF::PF() {}

void PF::STPT_Sort(Solution &sol) {
    if (sol.sequence.empty()) {
        sol.sequence.resize(this->processing_times.size());
        std::iota(sol.sequence.begin(), sol.sequence.end(), 0);
    }
    std::sort(sol.sequence.begin(), sol.sequence.end(), [this](size_t a, size_t b) {
        size_t sumA = 0, sumB = 0;
        for (auto t : this->processing_times[a])
            sumA += t;
        for (auto t : this->processing_times[b])
            sumB += t;
        return sumA < sumB;
    });
}

//---------------------------------------------------------
// Helper function to compute departure times for a given job sequence.
// The departure times are computed according to formulas (1)–(6) given above.
// We store, for each job in the sequence, a vector d[0..m] where:
//    d[0] is the starting time on the first machine (set to 0),
//    d[k] (for k = 1,.., m) is the departure time from machine k.
// (Recall: processing_times[job][k-1] is p(job,k) in the paper.)
//
// Input:
//    sequence - a permutation of job indices (order of processing)
//    processing_times - a matrix with processing_times[job][machine]
// Output:
//    A 2D vector d such that d[i][k] is the departure time of job sequence[i] on machine k,
//    with k = 0,1,...,m.
std::vector<std::vector<size_t>> computeDepartureTimes(
    const std::vector<size_t>& sequence,
    const std::vector<std::vector<size_t>>& processing_times)
{
    size_t n = sequence.size();
    assert(n > 0);
    size_t m = processing_times[0].size();

    // Cria uma matriz d com n linhas e (m+1) colunas, inicializada com zeros.
    std::vector<std::vector<size_t>> d(n, std::vector<size_t>(m + 1, 0));

    // ----- Para o primeiro job -----
    // Fórmula (1): d(π₁, 0) = 0.
    d[0][0] = 0;
    // Fórmula (2): d(π₁, k) = d(π₁, k-1) + p(π₁, k) para k = 1, …, m.
    for (size_t k = 1; k <= m; ++k) {
        d[0][k] = d[0][k - 1] + processing_times[sequence[0]][k - 1];
    }

    // ----- Para os jobs subsequentes (j = 2, …, n) -----
    for (size_t j = 1; j < n; ++j) {
        // Fórmula (3): d(πⱼ, 0) = d(πⱼ₋₁, 1)
        d[j][0] = d[j - 1][1];

        // Para máquinas 1 a m-1, use a fórmula (4):
        // d(πⱼ, k) = max { d(πⱼ, k-1) + p(πⱼ, k), d(πⱼ₋₁, k+1) }.
        for (size_t k = 1; k < m; ++k) {
            size_t option1 = d[j][k - 1] + processing_times[sequence[j]][k - 1];
            size_t option2 = d[j - 1][k + 1];
            d[j][k] = std::max(option1, option2);
        }
        // Fórmula (5): d(πⱼ, m) = d(πⱼ, m-1) + p(πⱼ, m).
        d[j][m] = d[j][m - 1] + processing_times[sequence[j]][m - 1];
    }

    return d;
}

Solution PF::createSolutionFromInstance(const Instance &instance) {
    Solution sol;
    size_t n = instance.num_jobs();
    size_t m = instance.num_machines();
    processing_times.resize(n, std::vector<size_t>(m, 0));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < m; ++j) {
            processing_times[i][j] = static_cast<size_t>(instance.p(i, j));
        }
    }
    return sol;
}

//---------------------------------------------------------
// PF heuristic implementation.
// This function builds a new job sequence by selecting, at each iteration,
// the candidate job (among unscheduled jobs) that minimizes the sum of idle and blocking times.
// (Here we use a measure sigma computed from the difference in departure times.)
Solution PF::solve(Solution &sol) {
    size_t n = processing_times.size();         // número de jobs
    size_t m = processing_times[0].size();        // número de máquinas

    // Passo 1: Ordena os jobs usando a regra STPT.
    if (sol.sequence.empty()) {
        STPT_Sort(sol);
    }

    // Vetores auxiliares para indicar os jobs já agendados e armazenar a nova sequência.
    std::vector<bool> scheduled(n, false);
    std::vector<size_t> newSeq;
    std::vector<size_t> unscheduled;

    // Passos 2 e 3: Seleciona o primeiro job (o primeiro na ordem STPT).
    size_t firstJob = sol.sequence.front();
    newSeq.push_back(firstJob);
    scheduled[firstJob] = true;

    // Preenche o conjunto de jobs não agendados.
    for (size_t i = 0; i < n; ++i) {
        if (!scheduled[i])
            unscheduled.push_back(i);
    }

    // Para cada posição na sequência (exceto a primeira), escolhe o job que minimiza a medida sigma.
    for (size_t i = 1; i < n; ++i) {
        // Calcula os departure times da sequência atual.
        std::vector<std::vector<size_t>> d_current = computeDepartureTimes(newSeq, processing_times);
        size_t bestSigma = std::numeric_limits<size_t>::max();
        size_t bestJob = unscheduled.front();  // valor inicial

        // Para cada job candidato não agendado:
        for (size_t candidate : unscheduled) {
            std::vector<size_t> candidateSequence = newSeq;
            candidateSequence.push_back(candidate);

            // Calcula os departure times da sequência candidata.
            std::vector<std::vector<size_t>> d_candidate = computeDepartureTimes(candidateSequence, processing_times);
            const std::vector<size_t>& d_new = d_candidate.back();

            // Calcula sigma como a soma, para cada máquina k (de 1 a m), de:
            //    d(candidate, k) - (d(current_last, k) + p(candidate, k))
            size_t sigma = 0;
            for (size_t k = 1; k <= m; ++k) {
                sigma += d_new[k] - (d_current.back()[k] + processing_times[candidate][k - 1]);
            }
            if (sigma < bestSigma) {
                bestSigma = sigma;
                bestJob = candidate;
            }
        } // fim do loop para candidatos

        // Adiciona o job selecionado à nova sequência.
        newSeq.push_back(bestJob);
        scheduled[bestJob] = true;
        unscheduled.erase(std::remove(unscheduled.begin(), unscheduled.end(), bestJob), unscheduled.end());
    } // fim do loop principal

    // Atualiza a solução: sequência, departure_times e custo (makespan)
    sol.sequence = newSeq;
    std::vector<std::vector<size_t>> d_final = computeDepartureTimes(newSeq, processing_times);
    sol.departure_times = d_final;
    sol.cost = d_final.back()[m];  // o makespan é o departure time do último job na última máquina

    return sol;
}
