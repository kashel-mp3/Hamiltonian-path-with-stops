#include <nlohmann/json.hpp>
#include "../utils.h"
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <fstream>
#include <climits>
#include <omp.h>

typedef std::vector<int> vec_int;
typedef std::vector<bool> vec_bool;
typedef std::vector<std::vector<int>> vec_vec_int;

using json = nlohmann::json;

int n;
int s;
int population_size = 10;
int number_of_generations = 25;
int elite_size = 1;
int num_parents = population_size / 2 - (population_size % 2);
vec_int stop_vertices;
vec_bool stop_vertices_check;
vec_vec_int graph;
std::ofstream res("genetic.txt");

bool is_valid_solution(const vec_int& solution){
    if(!stop_vertices_check[solution[0]] || !stop_vertices_check[solution.back()])
        return false;
    for(int i = 1; i < solution.size(); ++i)
        if(!graph[solution[i - 1]][solution[i]])
            return false;
    return true;
}

int fitness(const vec_int& solution) {
    int max_subpath = 0, cur_subpath = 0;
    for(int i = 1; i < n; ++i) {
        cur_subpath += graph[solution[i - 1]][solution[i]];
        if(stop_vertices_check[solution[i]]) {
            max_subpath = std::max(max_subpath, cur_subpath);
            cur_subpath = 0;
        }
    }
    return max_subpath;
}

void populate(vec_vec_int& population) {
    int new_solutions = population_size - population.size();
    vec_int base_permutation(n);
    for (int i = 0; i < n; ++i) base_permutation[i] = i;

    #pragma omp parallel
    {
        std::random_device rd;
        std::mt19937 rng(rd() + omp_get_thread_num());
        std::vector<vec_int> local_pop;

        #pragma omp for
        for (int i = 0; i < new_solutions; ++i) {
            vec_int perm;
            do {
                perm = base_permutation;
                std::shuffle(perm.begin(), perm.end(), rng);
            } while (!is_valid_solution(perm));
            local_pop.push_back(perm);
        }

        #pragma omp critical
        population.insert(population.end(), local_pop.begin(), local_pop.end());
    }
}

void mutate(vec_vec_int& population, const vec_bool& stop_vertices_check) {
    std::vector<int> fitness_vals(population.size());
    double total_fitness = 0.0;

    #pragma omp parallel for reduction(+:total_fitness)
    for (int i = 0; i < population.size(); ++i) {
        fitness_vals[i] = fitness(population[i]);
        total_fitness += fitness_vals[i];
    }

    #pragma omp parallel for
    for (int i = 0; i < population.size(); ++i) {
        std::mt19937 gen(std::random_device{}() + omp_get_thread_num());
        std::uniform_real_distribution<double> dis(0.0, 1.0);
        double prob = 1.0 - (double)fitness_vals[i] / total_fitness;
        if (dis(gen) <= prob) {
            vec_int& ind = population[i];
            std::uniform_int_distribution<int> dist(0, ind.size() - 1);
            int idx1 = dist(gen), idx2 = dist(gen);
            std::swap(ind[idx1], ind[idx2]);
        }
    }
}

vec_int crossover(const vec_int& A, const vec_int& B) {
    vec_int child;
    vec_bool used(n, false);
    for (int i = 0; i < n / 2; ++i) {
        child.push_back(A[i]);
        used[A[i]] = true;
    }
    for (int i = 0; i < n; ++i)
        if (!used[B[i]])
            child.push_back(B[i]);
    return child;
}

vec_vec_int rank_based_selection(const vec_vec_int& population) {
    vec_vec_int sorted = population;
    std::vector<int> fit_vals(population.size());

    #pragma omp parallel for
    for (int i = 0; i < population.size(); ++i)
        fit_vals[i] = fitness(population[i]);

    std::vector<size_t> indices(population.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return fit_vals[a] < fit_vals[b];
    });

    double total_rank = (population_size * (population_size + 1)) / 2.0;
    std::vector<double> prob(population_size);
    for (int i = 0; i < population_size; ++i)
        prob[i] = (i + 1) / total_rank;

    std::vector<double> cum_prob(population_size);
    cum_prob[0] = prob[0];
    for (int i = 1; i < population_size; ++i)
        cum_prob[i] = cum_prob[i - 1] + prob[i];

    std::vector<vec_int> selected;
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dis(0.0, 1.0);

    while (selected.size() < num_parents) {
        double p = dis(gen);
        for (int i = 0; i < population_size; ++i) {
            if (p <= cum_prob[i]) {
                selected.push_back(population[indices[i]]);
                break;
            }
        }
    }
    return selected;
}

void evolve_population(vec_vec_int& old_gen) {
    vec_vec_int parents = rank_based_selection(old_gen);
    vec_vec_int new_gen(old_gen.begin(), old_gen.begin() + elite_size);

    #pragma omp parallel
    {
        std::vector<vec_int> local_offspring;

        #pragma omp for nowait
        for (int i = 1; i < parents.size(); i += 2) {
            vec_int A = parents[i - 1], B = parents[i];
            vec_int Ar(A.rbegin(), A.rend()), Br(B.rbegin(), B.rend());
            vec_vec_int children = {
                crossover(A, B),
                crossover(B, A),
                crossover(Ar, B),
                crossover(Br, A)
            };
            for (auto& child : children) {
                if (is_valid_solution(child))
                    local_offspring.push_back(child);
            }
        }

        #pragma omp critical
        new_gen.insert(new_gen.end(), local_offspring.begin(), local_offspring.end());
    }

    populate(new_gen);
    mutate(new_gen, stop_vertices_check);

    // Sort by fitness and keep best individuals
    std::vector<std::pair<int, vec_int>> scored(new_gen.size());
    #pragma omp parallel for
    for (int i = 0; i < new_gen.size(); ++i)
        scored[i] = {fitness(new_gen[i]), new_gen[i]};

    std::sort(scored.begin(), scored.end(), [](auto& a, auto& b) {
        return a.first < b.first;
    });

    old_gen.clear();
    for (int i = 0; i < population_size; ++i)
        old_gen.push_back(scored[i].second);
}

bool is_connected(int n, vec_vec_int& g) {
    std::vector<bool> visited(n, false);
    std::function<void(int)> dfs = [&](int v) {
        visited[v] = true;
        for (int i = 0; i < n; ++i)
            if (g[v][i] && !visited[i])
                dfs(i);
    };
    dfs(0);
    return std::all_of(visited.begin(), visited.end(), [](bool v) { return v; });
}

int main(int argc, char** argv) {
    Utils utils;
    if (argc < 2) {
        std::cerr << "Provide path to input file.\n";
        return -1;
    }
    std::string input_path = argv[1];
    utils.read_data_from_json(input_path, n, s, graph, stop_vertices);

    if (!is_connected(n, graph)) {
        std::cout << -2 << '\n';
        return 0;
    }

    stop_vertices_check = vec_bool(n, false);
    for (int v : stop_vertices)
        stop_vertices_check[v] = true;

    vec_vec_int population;
    populate(population);

    for (int gen = 0; gen < number_of_generations; ++gen) {
        std::cout << "Generation " << gen + 1 << ":\n";
        evolve_population(population);
        for (const auto& ind : population)
            std::cout << "Fitness: " << fitness(ind) << '\n';
    }

    for (int i = 0; i < n; ++i)
        std::cout << population[0][i] << ' ';
    std::cout << "Fitness: " << fitness(population[0]) << '\n';

    return 0;
}
