#include "P_EDA.h"
#include "Core.h"
#include "Instance.h"
#include "Log.h"
#include "Parameters.h"
#include "RNG.h"
#include "Solution.h"
#include "constructions/NEH.h"
#include "constructions/PF.h"
#include "local-search/RLS.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <numeric>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <utility>

namespace {
    size_t uptime() {
        static const auto global_start_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - global_start_time);
        return duration.count();
    }
} // namespace

P_EDA::P_EDA(Instance &instance, Parameters &params, size_t ps, double lambda)
    : m_instance(instance), m_params(params), m_lambda(lambda), m_ps(ps) {
    m_pc.reserve(m_ps);
}

bool P_EDA::mrls(Solution &s, std::vector<size_t> &ref, Instance &instance) {
    bool improved = false;
    size_t n = instance.num_jobs();
    size_t j = 0;
    size_t cnt = 0;
    NEH helper(instance);
    while (cnt <= n) {
        j++;
        if(j >= n){
            j = j % n;
            std::vector<size_t> shuffled;
            shuffled.reserve(n);
            
            while(!ref.empty()) {
                const size_t k = RNG::instance().generate(size_t{0}, ref.size()-1);
        
                shuffled.push_back(ref[k]);
                ref.erase(ref.begin() + k);
            }

            ref = shuffled;
        }

        const size_t job = ref[j];
        for (size_t i = 0; i < s.sequence.size(); i++) {
            if (s.sequence[i] == job) {
                s.sequence.erase(s.sequence.begin() + (long)i);
                break;
            }
        }

        auto [best_index, makespan] = helper.taillard_best_insertion(s.sequence, job);
        s.sequence.insert(s.sequence.begin() + (long)best_index, job);

        if (makespan < s.cost) {
            cnt = 0;
            s.cost = makespan;
            improved = true;
            continue;
        }
        else{
            cnt++;
        }
    }

    return improved;
}

Solution P_EDA::solve() {
    // Set time limit to parameter or a default calculation
    size_t time_limit = 0;
    const size_t mxn = m_instance.num_jobs() * m_instance.num_machines();
    
    time_limit = (m_params.ro() * mxn);
    
    std::vector<size_t> ro;
    if (m_params.benchmark()) {
        time_limit = (100 * mxn); // RO == 100
        ro = {90, 60, 30};
    }
    // Shao ran tests with this duration, chaging ro parameter between 30,60,90

    size_t gen = 1;

    // generating the initial population and applying the modified linear rank selection
    generate_initial_population();
    modified_linear_rank_selection();
    std::vector<size_t> ref(m_instance.num_jobs()); // referencied shuffled sequence to mrls
    std::iota(ref.begin(), ref.end(), 0);

    // the p[i][j] represents how many times job j appeared before or in position i give the current population
    auto p = get_p();
    // the t[i][j][k] represents how many times job k appeared immideatly after job j in position i
    auto t = get_t();

    while (true) {
        const Solution alpha = probabilistic_model(p, t);

        if (m_params.verbose()) {
            std::cout << "alpha: \n";
            std::cout << alpha << "\n";
        }
        // getting the individual with the lowest makespan
        auto min_it = std::min_element(m_pc.begin(), m_pc.end(),
                                       [](const Solution &a, const Solution &b) { return a.cost < b.cost; });

        if (m_params.verbose()) {
            std::cout << "min: \n";
            std::cout << *min_it << "\n";
        }

        // the original individual is alpha and the goal is the lowest makespan individual
        Solution best = path_relink_swap(alpha, *min_it);

        if (m_params.verbose()) {
            std::cout << "path relink: \n";
            std::cout << best << "\n";
        }

        std::shuffle(ref.begin(), ref.end(), RNG::instance().gen());

        mrls(best, ref, m_instance);
        
        if (!ro.empty() && uptime() >= (ro.back() * mxn)) {
            auto min_sofar = std::min_element(m_pc.begin(), m_pc.end(), [](const Solution &a, const Solution &b) {
                return a.cost < b.cost; // Compare costs
            });
            std::cout << min_sofar[0].cost << '\n';
            ro.pop_back();
        }

        //  Program should not accept any solution if the time is out
        if (uptime() > time_limit) {
            break;
        }

        const auto [found_in_population, max_cost_pos] = in_population_and_max_makespan(best.sequence);
        const auto &max_cost = m_pc[max_cost_pos].cost;

        if (!found_in_population && best.cost < max_cost) {
            m_pc.erase(m_pc.begin() + max_cost_pos);
            m_pc.push_back(best);
        }

        gen = (gen + 1) % m_ps;
        if (gen == 0) {

            VERBOSE(m_params.verbose()) << "\ngen = 0...\n";

            // idk if this works, the if block is never executed...
            const auto &diversity = get_diversity();
            if (diversity < m_lambda) {

                if (m_params.verbose()) {
                    std::cout << "\nregenerating population...\n";
                }
                // it said in the arcticle we only recalculate P and T when we generated PS new individuals
                // but recalculating P and T only if we regenerate the population proved to generate better solutions
                population_regen();
                p = get_p();
                t = get_t();
            }
        }
        if (m_params.verbose()) {
            std::cout << "best sequence: ";
            for (auto &j : best.sequence) {
                std::cout << j << " ";
            }
            std::cout << "best: " << best.cost << "\n";
        }
    }

    auto min_it = std::min_element(m_pc.begin(), m_pc.end(), [](const Solution &a, const Solution &b) {
        return a.cost < b.cost; // Compare costs
    });
    return *min_it;
}

void P_EDA::generate_random_individuals() {

    std::vector<size_t> random_sequence(m_instance.num_jobs());
    std::iota(random_sequence.begin(), random_sequence.end(), 0);

    while (m_pc.size() < m_ps) {

        std::shuffle(random_sequence.begin(), random_sequence.end(), RNG::instance().gen());

        bool similar_individual = false;

        for (auto &ind : m_pc) {

            similar_individual = is_similar(random_sequence, ind.sequence, 0);

            if (similar_individual) {
                break;
            }
        }

        if (similar_individual) {
            continue;
        }

        Solution s;

        s.sequence = random_sequence;
        core::recalculate_solution(m_instance, s);
        m_pc.push_back(s);
    }
}

void P_EDA::generate_initial_population() {
    // this procedure will generate 0.1*PS pf-neh individuals, and the other 0.9*PS are randomly generated
    VERBOSE(m_params.verbose()) << "generating initial population...\n\n";
    const size_t n = m_instance.num_jobs();

    PF pf(m_instance);
    NEH neh(m_instance);

    const size_t lambda_pf_neh = std::min((size_t)25, n);

    std::vector<size_t> sorted_jobs = core::stpt_sort(m_instance);

    size_t l = 0;

    const size_t pf_neh_individuals = m_ps / 10;

    while (m_pc.size() < pf_neh_individuals && l < n) {

        // Taking into account that PF-NEH is a deterministic algorithm
        // Shao decides to change the first job in the pf heuristic in each individual
        // he does this by creating a solution with the jobs sorted by smallest total processing time(stpt)
        // and for each job in this solution, he applies the the pf heuristic with this job (and then NEH ofc)
        // this will go on until population has 0.1*PS individuals

        Solution s;
        const size_t first_job = sorted_jobs[l];

        pf.pf_insertion_phase(s, first_job);

        std::vector<size_t> candidate_jobs = {s.sequence.begin() + (long)(n - lambda_pf_neh + 1), s.sequence.end()};

        s.sequence = {s.sequence.begin(),
                      s.sequence.begin() +
                          (long)(n - lambda_pf_neh + 1)}; // needs to be n - lambda + 1 because [first_pos, last_post)

        neh.second_step(std::move(candidate_jobs), s);

        m_pc.push_back(s);

        l++;
    }

    // generating the 0.9*PS remaining individuals randomly
    generate_random_individuals();
    assert(m_pc.size() == m_ps);
}

void P_EDA::modified_linear_rank_selection() {
    // this procedure will generate another population (also with PS individuals)
    // based on the population we just generated.
    // First it will sort and rank the jobs based on lowest makespan, i.e.,
    // the job with the lowest makespan has the highest rank

    std::vector<double> sum_probabilities(m_ps + 1);

    size_t l = m_ps - 1;
    // sorting individuals by descending makespan
    auto sort_criteria = [](Solution &a, Solution &b) { return a.cost > b.cost; };

    std::sort(m_pc.begin(), m_pc.end(), sort_criteria);

    const double sum_of_ranks = (double)(1 + m_ps) * m_ps / 2;
    sum_probabilities[0] = 0;

    // after sorting, we make a sequence which sums the probabilities from 1 to PS
    // so the highest ranks have more probabilitie of beign chosen
    for (size_t i = 1; i <= m_ps; i++) {
        sum_probabilities[i] = sum_probabilities[i - 1] + (double)i / sum_of_ranks;
    }

    std::vector<Solution> new_pc;
    new_pc.reserve(m_ps);

    while (new_pc.size() < m_ps) {
        // here we generate 2 numbers so we can choose between 2 procedures
        const double lambda1 = RNG::instance().generate_real_number(0, 1);
        const double lambda2 = RNG::instance().generate_real_number(0, 1);

        if (lambda1 < lambda2) {
            // PROCEDURE 1 --------
            // adding the best current solution to the new population and going to the next
            new_pc.push_back(m_pc[l]);
            l--;
        } else {
            // PROCEDURE 2 --------
            // we generate a random number sigma and we will iterate to the sum of probabilities sequence
            // and search the range of proabibilities that contains sigma
            // so if we are lucky we will get good individuals
            const double sigma = RNG::instance().generate_real_number(0, 1);

            for (size_t i = 1; i <= m_ps; i++) {

                if (sum_probabilities[i - 1] <= sigma && sigma < sum_probabilities[i]) {
                    new_pc.push_back(m_pc[i - 1]);
                    break;
                }
            }
        }
    }
    
    m_pc = new_pc;
}

Solution P_EDA::probabilistic_model(const SizeTMatrix &p, const std::vector<SizeTMatrix> &t) {
    // in this probabilistic model, the objective is to get an individual that represents
    // accuratly the current population

    const size_t n = m_instance.num_jobs();

    std::vector<size_t> unasigned_jobs(m_instance.num_jobs());
    std::iota(unasigned_jobs.begin(), unasigned_jobs.end(), 0);
    std::vector<size_t> final_sequence;
    final_sequence.reserve(n);

    while (final_sequence.size() < n - 1) {
        // this probability vector has the size of the unasigned jobs
        // each element of this vector represents the probability of adding the job in that position
        // to the current solution
        auto probabilities = get_probability_vector(final_sequence, unasigned_jobs, p, t);

        double roulette_wheel = 0;

        size_t job_to_insert = unasigned_jobs[0];

        // this roulette wheel works the same way as the sum of probabilities
        // in the modified linear rank selection
        const double r = RNG::instance().generate_real_number(0, 1);

        for (size_t j = 0; j < unasigned_jobs.size(); j++) {

            if (roulette_wheel <= r && r < roulette_wheel + probabilities[j]) {
                job_to_insert = unasigned_jobs[j];
            }
            roulette_wheel += probabilities[j];
        }
        final_sequence.push_back(job_to_insert);
        unasigned_jobs.erase(std::remove(unasigned_jobs.begin(), unasigned_jobs.end(), job_to_insert));
    }
    final_sequence.push_back(unasigned_jobs.front());

    Solution s;
    s.sequence = final_sequence;
    core::recalculate_solution(m_instance, s);
    return s;
}

std::vector<std::vector<size_t>> P_EDA::get_p() {
    const size_t n = m_instance.num_jobs();

    std::vector<std::vector<size_t>> p(n, std::vector<size_t>(n, 0));
    for (const auto &s : m_pc) {
        const auto &sequence = s.sequence;

        for (size_t i = 0; i < n; i++) {

            p[i][sequence[i]]++;
        }
    }

    for (size_t j = 0; j < n; j++) {
        for (size_t i = 1; i < n; i++) {
            p[i][j] += p[i - 1][j];
        }
    }
    return p;
}

std::vector<SizeTMatrix> P_EDA::get_t() {
    const size_t n = m_instance.num_jobs();
    std::vector<SizeTMatrix> t(n);

    for (auto &lambda_jk : t) {
        lambda_jk = std::vector<std::vector<size_t>>(n, std::vector<size_t>(n, 0));
    }

    for (auto &s : m_pc) {
        const auto &sequence = s.sequence;
        for (size_t i = 1; i < n; i++) {
            t[i][sequence[i - 1]][sequence[i]]++;
        }
    }

    return t;
}

std::vector<double> P_EDA::get_probability_vector(const std::vector<size_t> &sequence,
                                                  const std::vector<size_t> &unasigned_jobs, const SizeTMatrix &p,
                                                  const std::vector<SizeTMatrix> &t) {
    const size_t n = m_instance.num_jobs();
    std::vector<double> probabilities(unasigned_jobs.size());

    if (sequence.empty()) {

        double sum_p = 0;
        for (const auto &j : unasigned_jobs) {
            sum_p += (double)p[0][j];
        }

        for (size_t j = 0; j < n; j++) {
            const size_t job = unasigned_jobs[j];
            probabilities[j] = (double)p[0][job] / sum_p;
        }
    } else {

        const size_t last_job = sequence.back();
        const size_t pos = sequence.size(); // the position the new job will be inserted

        double sum_p = 0;
        double sum_t = 0;
        for (const auto &j : unasigned_jobs) {
            sum_p += (double)p[pos][j];
            sum_t += (double)t[pos][last_job][j];
        }

        for (size_t j = 0; j < unasigned_jobs.size(); j++) {
            const size_t job = unasigned_jobs[j];

            double n_i_j_k = -1;

            if (sum_t == 0) {
                n_i_j_k = 1 / (double)unasigned_jobs.size();
            } else {
                n_i_j_k = (double)t[pos][last_job][job] / sum_t;
            }

            probabilities[j] = ((double)p[pos][job] / sum_p) + n_i_j_k;
            probabilities[j] /= 2;
        }
    }

    return probabilities;
}

Solution P_EDA::path_relink_swap(const Solution &alpha, const Solution &beta) {
    Solution best;
    Solution current = alpha;
    const size_t n = m_instance.num_jobs();

    /*
    Or the difference is 0 (and the solution are equal), or is greater than or equal to 2.
    If is less than or equal to 2, maybe one swap can make the solutions equal, hence it's
    applied a mutation.
    */
    size_t difference = 0;
    for (size_t k = 0; k < n; k++) {
        if (alpha.sequence[k] != beta.sequence[k]) {

            difference++;
            if (difference > 2) {
                break;
            }
        }
    }

    if (difference <= 2) {
        mutation(current);
    }

    // cnt = number of jobs that are already in the correct position
    /*
    This part iterates over solution pi, finding where job i from solution
    beta is in solution pi. If i = j (j is the index of the job searched for
    in solution pi), then the jobs are in the same position (hence the correct
    position) and i = i+1 and we search for the next job. Otherwise, we swap
    the job at position i with the job at position j in solution beta, and
    move on to the next iteration.
    */
    size_t i = 0;
    for (size_t cnt = 0; cnt < n; cnt++) {

        if (current.sequence[i] == beta.sequence[i]) {
            i++;
            continue;
        }

        const size_t job = current.sequence[i];
        for (size_t j = i + 1; j < n; j++) {

            if (job != beta.sequence[j]) {
                continue;
            }

            std::swap(current.sequence[i], current.sequence[j]);
            core::recalculate_solution(m_instance, current);

            if (current.cost < best.cost) {
                best = current; // new best interdiary solution
            }

            break;
        }
    }

    return best;
}

inline void P_EDA::mutation(Solution &individual) {

    const size_t insertion_position = RNG::instance().generate((size_t)0, individual.sequence.size() - 1);
    size_t job_position = insertion_position;

    while (insertion_position == job_position) {
        job_position = RNG::instance().generate((size_t)0, individual.sequence.size() - 1);
    }

    individual.sequence.insert(individual.sequence.begin() + insertion_position, individual.sequence[job_position]);

    if (insertion_position > job_position) {
        individual.sequence.erase(individual.sequence.begin() + job_position);
    } else {
        individual.sequence.erase(individual.sequence.begin() + job_position + 1);
    }

    core::recalculate_solution(m_instance, individual);
}

std::vector<size_t> P_EDA::fisher_yates_shuffle() {

    std::vector<size_t> sequence(m_instance.num_jobs());
    std::iota(sequence.begin(), sequence.end(), 0);

    for (size_t i = sequence.size() - 1; i > 0; i--) {
        const size_t j = RNG::instance().generate(size_t{0}, i);

        std::swap(sequence[i], sequence[j]);
    }
    return sequence;
}

std::pair<bool, size_t> P_EDA::in_population_and_max_makespan(std::vector<size_t> &sequence) {
    // returns if the sequence is in the current population and also the position of the individual with the highest
    // makespan

    size_t max_cost = 0;
    size_t max_cost_pos = 0;

    for (size_t i = 0; i < m_ps; i++) {
        const auto &ind = m_pc[i];
        const size_t current_cost = ind.cost;
        if (current_cost > max_cost) {
            max_cost = current_cost;
            max_cost_pos = i;
        }

        bool found = true;
        const auto &seq = ind.sequence;

        for (size_t j = 0; j < m_instance.num_jobs(); j++) {
            if (seq[j] != sequence[j]) {
                found = false;
                break;
            }
        }
        if (found) {
            return {true, max_cost_pos};
        }
    }

    return {false, max_cost_pos};
}

double P_EDA::get_diversity() {
    // idk if this really works

    const auto n = m_instance.num_jobs();
    double diversity = 0;

    SizeTMatrix c(n, std::vector<size_t>(n, 0));

    for (const auto &s : m_pc) {
        const auto &sequence = s.sequence;

        for (size_t i = 0; i < n; i++) {

            c[i][sequence[i]]++;
        }
    }

    for (size_t k = 0; k < n; k++) {
        for (size_t alpha = 0; alpha < n; alpha++) {

            diversity += ((double)c[k][alpha] / m_ps) * (1 - ((double)c[k][alpha] / m_ps));
        }
    }
    VERBOSE(m_params.verbose()) << "diversity without div: " << diversity << "\n";

    diversity /= (double)n - 1;

    return diversity;
}
void P_EDA::population_regen() {
    auto sort_criteria = [](Solution &a, Solution &b) { return a.cost < b.cost; };

    std::sort(m_pc.begin(), m_pc.end(), sort_criteria);

    // removing last 40% jobs of the population
    m_pc = {m_pc.begin(), m_pc.end() - 2 * (m_ps / 5)};

    // keep 20% of the population and do a random insertion in the 40% intermediates
    for (size_t j = (m_ps / 5) + 1; j < m_pc.size(); j++) {
        auto &current_individual = m_pc[j];
        auto &current_seq = current_individual.sequence;

        // random reinsert
        auto pos = RNG::instance().generate((size_t)0, current_seq.size() - 1);
        auto job = current_seq[pos];
        current_seq.erase(current_seq.begin() + pos);
        pos = RNG::instance().generate((size_t)0, current_seq.size());
        current_seq.insert(current_seq.begin() + pos, job);
        core::recalculate_solution(m_instance, current_individual);
    }
    generate_random_individuals();
}

inline bool P_EDA::is_similar(const std::vector<size_t> &a, const std::vector<size_t> &b, size_t similarity_threshold) {

    size_t diference = 0;
    for (size_t j = 0; j < a.size(); j++) {

        if (a[j] != b[j]) {
            diference++;
            if (diference > similarity_threshold) {
                return false;
            }
        }
    }
    return true;
}
void P_EDA::print_pc() const {
    std::cout << "Population: \n";
    for (size_t i = 0; i < m_pc.size(); i++) {
        std::cout << "individual " << i << ": \n" << m_pc[i] << "\n";
    }
}
