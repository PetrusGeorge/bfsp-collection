#include "PF_NEH.h"
#include <cassert>
#include <iostream>
#include <numeric>

PF_NEH::PF_NEH() {}

Solution PF_NEH::solve(int x, int delta) {
    PF pf;

    size_t nJobs = processing_times.size();
    size_t m = processing_times[0].size();

    // Cria o vetor de índices (0, 1, ..., nJobs-1) e ordena com a regra STPT.
    std::vector<size_t> sortedJobs(nJobs);
    std::iota(sortedJobs.begin(), sortedJobs.end(), 0);
    // Reutiliza sua função STPT_Sort – como ela age sobre uma Solution,
    // criamos uma solução temporária e a usamos apenas para obter a ordem.
    Solution tempSol;
    tempSol.sequence = sortedJobs;  // Inicialmente, a sequência é a ordem natural
    // Aqui, supondo que STPT_Sort ordene in-place (você já tem essa função implementada)
    pf.STPT_Sort(tempSol);
    sortedJobs = tempSol.sequence;  // Agora, sortedJobs contém os índices ordenados pelo tempo total

    // Vetor para armazenar as soluções candidatas geradas.
    std::vector<Solution> candidateSolutions;

    // Para cada candidato de primeiro job (h = 0 até x-1)
    for (int h = 0; h < x && h < static_cast<int>(sortedJobs.size()); h++) {
        // Cria uma solução candidata: comece com a ordem STPT
        Solution candSol;
        candSol.sequence = sortedJobs;  
        // Força que o primeiro job seja o h-ésimo (ou seja, troca o job da posição 0 com o da posição h)
        std::swap(candSol.sequence[0], candSol.sequence[h]);

        // Chama PF para gerar a sequência completa.
        // Como PF verifica se a sequência já não está vazia, ela usará a sequência que você definiu.
        candSol = pf.solve(candSol);

        // --- Melhoria local: reinserção dos últimos delta jobs ---
        // Se delta for maior que o número de jobs, usa todos.
        size_t startIndex = (delta > static_cast<int>(candSol.sequence.size()))
                                ? 0 
                                : candSol.sequence.size() - delta;
        // Para cada posição na região final da sequência:
        for (size_t pos = startIndex; pos < candSol.sequence.size(); pos++) {
            size_t job = candSol.sequence[pos];
            // Remove o job da posição pos.
            std::vector<size_t> tempSeq = candSol.sequence;
            tempSeq.erase(tempSeq.begin() + pos);

            size_t bestCmax = std::numeric_limits<size_t>::max();
            std::vector<size_t> bestSeq;
            // Testa inserir o job em todas as posições possíveis.
            for (size_t j = 0; j <= tempSeq.size(); j++) {
                std::vector<size_t> candidateSeq = tempSeq;
                candidateSeq.insert(candidateSeq.begin() + j, job);
                // Avalia a sequência: calcula os departure times e o makespan
                auto dCandidate = computeDepartureTimes(candidateSeq, processing_times);
                size_t candidateCmax = dCandidate.back()[m]; // makespan é o último valor da última máquina
                if (candidateCmax < bestCmax) {
                    bestCmax = candidateCmax;
                    bestSeq = candidateSeq;
                }
            }
            // Atualiza a sequência candidata com a melhor inserção encontrada.
            candSol.sequence = bestSeq;
        }
        // Recalcula os departure times e o custo (makespan) da solução candidata.
        auto dFinal = computeDepartureTimes(candSol.sequence, processing_times);
        candSol.departure_times = dFinal;
        candSol.cost = dFinal.back()[m];

        candidateSolutions.push_back(candSol);
    }

    // Seleciona a melhor solução dentre as candidatas (a de menor makespan).
    Solution bestSolution = candidateSolutions.front();
    for (const auto &solCand : candidateSolutions) {
        if (solCand.cost < bestSolution.cost)
            bestSolution = solCand;
    }

    return bestSolution;
}