#include "algorithms/Grasp_NEH.h"
#include "Instance.h"
#include "algorithms/GRASP.h"
#include "Core.h"

#include <bits/types/cookie_io_functions_t.h>
#include <cassert>
#include <numeric>

Solution GraspNeh::solve(int x, int delta, const Instance &instance) {

    size_t n_jobs = instance.num_jobs();
    size_t m = instance.num_machines();

    // Cria o vetor de índices (0, 1, ..., nJobs-1) e ordena com a regra STPT.
    std::vector<size_t> sorted_jobs(n_jobs);
    std::iota(sorted_jobs.begin(), sorted_jobs.end(), 0);
    // Reutiliza sua função STPT_Sort – como ela age sobre uma Solution,
    // criamos uma solução temporária e a usamos apenas para obter a ordem.

    GRASP grasp(instance);

    sorted_jobs = core::stpt_sort(instance);

    // Vetor para armazenar as soluções candidatas geradas.
    std::vector<Solution> candidate_solutions;

    // Para cada candidato de primeiro job (h = 0 até x-1)
    for (int h = 0; h < x && h < static_cast<int>(sorted_jobs.size()); h++) {
        // Cria uma solução candidata: comece com a ordem STPT
        Solution cand_sol;
        cand_sol.sequence = sorted_jobs;
        // Força que o primeiro job seja o h-ésimo (ou seja, troca o job da posição 0 com o da posição h)
        std::swap(cand_sol.sequence[0], cand_sol.sequence[h]);

        // Chama GRASP para gerar a sequência completa.
        // Como GRASP verifica se a sequência já não está vazia, ela usará a sequência que você definiu.
        cand_sol = grasp.solve(delta);

        // --- Melhoria local: reinserção dos últimos delta jobs ---
        // Se delta for maior que o número de jobs, usa todos.
        size_t start_index = (delta > static_cast<int>(cand_sol.sequence.size())) ? 0 : cand_sol.sequence.size() - delta;
        // Para cada posição na região final da sequência:
        for (size_t pos = start_index; pos < cand_sol.sequence.size(); pos++) {
            size_t job = cand_sol.sequence[pos];
            // Remove o job da posição pos.
            std::vector<size_t> temp_seq = cand_sol.sequence;
            temp_seq.erase(temp_seq.begin() + (long)pos);

            size_t best_cmax = std::numeric_limits<size_t>::max();
            std::vector<size_t> best_seq;
            // Testa inserir o job em todas as posições possíveis.
            for (size_t j = 0; j <= temp_seq.size(); j++) {
                std::vector<size_t>& candidate_seq = temp_seq;
                candidate_seq.insert(candidate_seq.begin() + (long)j, job);
                // Avalia a sequência: calcula os departure times e o makespan
                auto d_candidate = core::calculate_departure_times(instance, candidate_seq);
                size_t candidate_cmax = d_candidate.back()[m]; // makespan é o último valor da última máquina
                if (candidate_cmax < best_cmax) {
                    best_cmax = candidate_cmax;
                    best_seq = candidate_seq;
                }
            }
            // Atualiza a sequência candidata com a melhor inserção encontrada.
            cand_sol.sequence = best_seq;
        }
        // Recalcula os departure times e o custo (makespan) da solução candidata.
        auto d_final = core::calculate_departure_times(instance, cand_sol.sequence);
        cand_sol.departure_times = d_final;
        cand_sol.cost = d_final.back()[m];

        candidate_solutions.push_back(cand_sol);
    }

    // Seleciona a melhor solução dentre as candidatas (a de menor makespan).
    Solution best_solution = candidate_solutions.front();
    for (const auto &sol_cand : candidate_solutions) {
        if (sol_cand.cost < best_solution.cost) {
            best_solution = sol_cand;
        }
    }

    return best_solution;
}
