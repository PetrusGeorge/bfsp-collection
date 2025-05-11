#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <chrono>


#include "Instance.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"
#include "constructions/GRASP.h"
#include "constructions/LPT.h"
#include "constructions/NEH.h"
#include "constructions/PF.h"
#include "constructions/Grasp_NEH.h"
#include "constructions/PF_NEH.h"
#include "constructions/PW.h"
#include "constructions/mNEH.h"
#include "constructions/MinMax.h"
#include "Core.h"
#include "local-search/RLS.h"

struct Individual {
    int ds;                  // Destruction size
    int ps;                  // Perturbation strength
    double tau;              // Temperature adjustment
    double jP;               // Jumping probability
    std::vector<size_t> permutation;  // Job permutation
    double fitness;          // Makespan or objective value

    std::vector<double> asParameters() const {
        return {static_cast<double>(ds), static_cast<double>(ps), tau, jP};
    }
};

static auto T0 = std::chrono::high_resolution_clock::now();
double elapsed_cpu_time_ms() {
    auto now = std::chrono::high_resolution_clock::now();
    // Calcula a diferença em milissegundos
    return std::chrono::duration<double, std::milli>(now - T0).count();
}

// -------------------------------------------------------------------
// 1) DESTRUCTION–CONSTRUCTION (IG): remove ds jobs e os reinsere
//    um a um na melhor posição segundo o critério de inserção NEH.
// -------------------------------------------------------------------

Solution destruct_construct(
    const std::vector<size_t>& pi,   // permutação original
    int ds,                          // destruction size
    NEH& neh                         // objeto NEH já inicializado
) {
    
    Solution s;
    s.sequence = pi; // cópia da sequência original
    s.cost = std::numeric_limits<size_t>::max();

    // 2. seleciona ds trabalhos sem repetição
    std::vector<size_t> removed;
    std::vector<size_t> indices(pi.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), RNG::instance().gen());
    for (int i = 0; i < ds; ++i) {
        removed.push_back(s.sequence[indices[i]]);
    }

    // 3. retira esses jobs da sequência
    std::sort(indices.begin(), indices.begin() + ds, std::greater<>());
    for (int i = 0; i < ds; ++i) {
        s.sequence.erase(s.sequence.begin() + indices[i]);
    }

    // 4. re-insere cada job na melhor posição (já calcula o makespan)
    for (size_t job : removed) {
        auto [bestPos, makespan] = neh.taillard_best_insertion(s.sequence, job);
        s.sequence.insert(s.sequence.begin() + bestPos, job);
        s.cost = makespan; // Atualiza o custo a cada inserção
    }

    return s;
}

// -------------------------------------------------------------------
// 2) PERTURBAÇÃO (ILS): remove ps jobs e os reinsere em posições
//    aleatórias, criando uma pequena “bagunça” na solução.
// -------------------------------------------------------------------

Solution perturbation(
    const std::vector<size_t>& pi,  // permutação original
    int ps,                         // perturbation strength
    Instance& instance         // instância para cálculo do makespan
) {
    Solution s;
    s.sequence = pi; // cópia da sequência original

    // 2. seleciona ps posições distintas e retira esses jobs
    std::vector<size_t> removed;
    std::vector<size_t> indices(pi.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), RNG::instance().gen());
    for (int i = 0; i < ps; ++i) {
        removed.push_back(s.sequence[indices[i]]);
    }

    // 3. remove do maior índice para o menor
    std::sort(indices.begin(), indices.begin() + ps, std::greater<>());
    for (int i = 0; i < ps; ++i) {
        s.sequence.erase(s.sequence.begin() + indices[i]);
    }

    // 4. reinsere aleatoriamente e calcula o makespan final
    std::uniform_int_distribution<size_t> distPos(0, s.sequence.size());
    for (size_t job : removed) {
        size_t pos = distPos(RNG::instance().gen());
        s.sequence.insert(s.sequence.begin() + pos, job);
    }

    // Calcula o makespan após a perturbação
    auto d = core::calculate_departure_times(instance, s.sequence);
    s.cost = d.back().back();

    return s;
}

int main(int argc, char *argv[]) {

    const Parameters params(argc, argv);
    Instance instance(params.instance_path());

    /*try {*/
    /*    Instance instance(params.instance_path());*/
    /*} catch (std::runtime_error &err) {*/
    /*    std::cerr << err.what() << '\n';*/
    /*    exit(EXIT_FAILURE);*/
    /*}*/

    if (auto seed = params.seed()) {
        RNG::instance().set_seed(*seed);
    }

    int n_jobs = instance.num_jobs();
    int n_machines = instance.num_machines();
    std::vector<size_t> phi(n_jobs);
    std::iota(phi.begin(), phi.end(), 0);

    int NP = params.np(); // Tamanho da população

    double total_processing_time = instance.total_processing_time();
    // Initialize Population
    std::vector<Individual> population(NP); // Inicializa a população com NP indivíduos

    NEH neh(instance);

    PF_NEH pf_neh(instance);

    for (int i = 0; i < NP; ++i) {
        Individual& ind = population[i];

        ind.ds = RNG::instance().generate(params.min_ds(), params.max_ds());
        ind.ps = RNG::instance().generate(params.min_ps(), params.max_ps());
        ind.tau = RNG::instance().generateReal(params.min_tau(), params.max_tau());
        ind.jP = RNG::instance().generateReal(params.min_jP(), params.max_jP());

        // The first individual is constructed with the PF_NEH algorithm

        if (i == 0){
            size_t lambda = 15; // Ele usa lambda = 15 para o pf_neh
            Solution sol = pf_neh.solve(lambda);
            ind.permutation = sol.sequence;
            ind.fitness = sol.cost;
        }

        // The second individual is constructed with the GRASP_NEH algorithm
        else if (i == 1){
            Solution sol = GraspNeh::solve(15, 0.0005, instance);// x = 15 and beta = 0.0005
            ind.permutation = sol.sequence;
            ind.fitness = sol.cost;
        }
        // Demais indivíduos: permutação aleatória + segunda fase do NEH
        else {
            std::vector<size_t> random_phi = phi;
            std::shuffle(random_phi.begin(), random_phi.end(),
                     RNG::instance().gen());
            
            Solution sol = neh.solve(random_phi); // Verifiar se isso é o correto ou só fazer a segunda fase
            ind.permutation = sol.sequence;
            ind.fitness = sol.cost;
        }

        population[i] = ind;

    }

    // ——— Loop de evolução do DE-PLS ———
    while(true){
        
        if (elapsed_cpu_time_ms() > 10 * n_jobs * n_machines) {
            break;
        }
        
        for (int i = 0; i < NP; ++i){
            // Seleciona três indivíduos aleatórios da população, a,b e c != i
            int a, b, c;
            do {
                a = RNG::instance().generate(0, NP - 1);
            } while (a == i);
            do {
                b = RNG::instance().generate(0, NP - 1);
            } while (b == i || b == a);
            do {
                c = RNG::instance().generate(0, NP - 1);
            } while (c == i || c == a || c == b);

            // Geração de um vetor mutante v_i
            std::vector<double> v(4), x_i(4), x_a(4), x_b(4), x_c(4);

            // Monta vetores de parâmetros dos indivíduos
            x_i = population[i].asParameters();
            x_a = population[a].asParameters();
            x_b = population[b].asParameters();
            x_c = population[c].asParameters();

            const double F = params.mutation_factor(); // Fator de mutação
            for (int k = 0; k < 4; ++k) {
                v[k] = x_a[k] + F * (x_b[k] - x_c[k]);
            }

            // Crossover binomial -> trial u_i
            std::vector<double> u(4);
            double CR = params.crossover_rate(); // Taxa de crossover
            int dj = RNG::instance().generate(0, 3); // Garante ao menos um gene de x_i
            for (int k = 0; k < 4; ++k) {
                double r = RNG::instance().generateReal(0.0, 1.0);
                if (r <= CR || k == dj) u[k] = v[k];
                else u[k] = x_i[k];
            }
            
            // Repara parâmetros fora do intervalo
            auto clamp = [&](int k, double& val){
                double mn, mx;
                switch (k) {
                    case 0: mn = params.min_ds(); mx = params.max_ds(); break;
                    case 1: mn = params.min_ps(); mx = params.max_ps(); break;
                    case 2: mn = params.min_tau(); mx = params.max_tau(); break;
                    case 3: mn = params.min_jP(); mx = params.max_jP(); break;
                    default: mn = 0; mx = 1;           break;
                }
                if (val < mn || val > mx)
                    val = mn + RNG::instance().generateReal(0.0, 1.0) * (mx - mn);
            };

            for (int k = 0; k < 4; ++k) clamp(k, u[k]);

            // Extrai ds, ps, tau e jP do vetor u
            Individual trial;
            trial.ds = static_cast<int>(u[0]);
            trial.ps = static_cast<int>(u[1]);
            trial.tau = u[2];
            trial.jP = u[3];

            // Gera a solução pi_u^i a partir de pi_i^-1 (pertub ou destruct+construct)
            Solution trial_sol;
            if(RNG::instance().generateReal(0.0, 1.0) < trial.jP){
                trial_sol = perturbation(population[i].permutation, trial.ps, instance);
            } else {
                trial_sol = destruct_construct(population[i].permutation,
                                           trial.ds,
                                           neh);
            }

            // Aplica o RLS para obter pi_new
            
            rls(trial_sol, trial_sol.sequence, instance);

            trial.permutation = std::move(trial_sol.sequence);
            trial.fitness = trial_sol.cost;
            
            // Seleção: aceita se melhorar ou por prob. de SA
            
            double f_old = population[i].fitness;
            double f_new = trial.fitness;

            if (f_new <= f_old) {
                population[i] = trial;
            } else {
                double T = (total_processing_time / (10.0 * n_jobs * n_machines)) * trial.tau;
                double prob = std::exp((f_new - f_old) / T);
                if (RNG::instance().generateReal(0.0,1.0) < prob) {
                    population[i] = trial;
                }
            }
        }
    }

    // Retorna a melhor solução encontrada
    auto best = *std::min_element(
        population.begin(), population.end(),
        [](auto &A, auto &B){ return A.fitness < B.fitness; }
    );
    std::cout << "Best makespan = " << best.fitness << "\n";
    std::cout << "Best sequence: ";
    for (auto j : best.permutation) std::cout << j << ' ';
    std::cout << '\n';
    
}
