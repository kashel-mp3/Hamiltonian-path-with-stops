#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <fstream>
#include <climits>

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

bool is_valid_solution(vec_int solution){
    int size = solution.size();
    if(!stop_vertices_check[solution[0]] || !stop_vertices_check[solution[size - 1]]) {
        return 0;
    }
    for(int i = 1; i < size; ++i) {
        if(!graph[solution[i - 1]][solution[i]]) {
            return 0;
        }
    }
    return 1;
}

int fitness(vec_int solution) {
    int max_subpath = 0;
    int cur_subpath = 0;
    for(int i = 1; i < n; ++i) {
        cur_subpath += graph[solution[i - 1]][solution[i]];
        if(stop_vertices_check[solution[i]]) {
            if(max_subpath < cur_subpath) {
                max_subpath = cur_subpath;
            }
            cur_subpath = 0;
        }
    }
    return max_subpath;
}

void populate(vec_vec_int& population){
    int new_solutions = population_size - population.size();
    vec_int permutation(n);
    for(int i = 0; i < n; ++i) {
        permutation[i] = i;
    }
    std::random_device rd;
    std::default_random_engine rng(rd());
    for(int i = 0; i < new_solutions; ++i) {
        do {
            std::shuffle(permutation.begin(), permutation.end(), rng);
        } while(!is_valid_solution(permutation));
        population.push_back(permutation);
    }
}

void mutate(std::vector<std::vector<int>>& population, const std::vector<bool>& stop_vertices_check) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(0.0, 1.0);

    double total_fitness = 0.0;
    for(vec_int member : population) {
        total_fitness += fitness(member);
    }

    for(int i = 0; i < population.size(); ++i) {
        double mutation_probability = 1.0 - (static_cast<double>(fitness(population[i])) / total_fitness);

        if(dis(gen) <= mutation_probability) {
            std::vector<int>& individual = population[i];
            std::uniform_int_distribution<int> dist(0, individual.size() - 1);
            int swap_position = dist(gen);
            if (stop_vertices_check[individual[swap_position]]) {
                std::vector<int> other_ones;
                for (int j = 0; j < individual.size(); ++j) {
                    if (stop_vertices_check[individual[j]] && j != swap_position) {
                        other_ones.push_back(j);
                    }
                }

                if (!other_ones.empty()) {
                    std::uniform_int_distribution<int> other_dist(0, other_ones.size() - 1);
                    int other_index = other_dist(gen);
                    int other_pos = other_ones[other_index];
                    std::swap(individual[swap_position], individual[other_pos]);
                }
            } else if (stop_vertices_check[individual[swap_position]]) {
                std::uniform_int_distribution<int> other_dist(0, individual.size() - 1);
                int other_pos = other_dist(gen);
                std::swap(individual[swap_position], individual[other_pos]);
            } else {
                int start_pos = 1;
                int end_pos = individual.size() - 2;
                std::uniform_int_distribution<int> start_dist(start_pos, end_pos);
                int start_swap_pos = start_dist(gen);
                std::swap(individual[swap_position], individual[start_swap_pos]);
            }
        }
    }
}

vec_int crossover(vec_int parent_A, vec_int parent_B) {
    vec_int offspring;
    vec_bool included(n, 0);
    for(int i = 0; i < n / 2; ++i) {
        if(parent_A[i] != parent_B[n - 1]) {
            offspring.push_back(parent_A[i]);
            included[parent_A[i]] = 1;
        }
    }
    for(int i = 0; i < n; ++i) {
        if(!included[parent_B[i]]) {
            offspring.push_back(parent_B[i]);
        }
    }
    return offspring;
}

vec_vec_int rank_based_selection(vec_vec_int& population) {
    vec_vec_int selected_parents;
    std::sort(population.begin(), population.end(), [](const vec_int& a, const vec_int& b) {
        return fitness(a) < fitness(b);
    });
    double total_rank = (population_size * (population_size + 1)) / 2.0;
    std::vector<double> selection_probabilities(population_size);
    for (int i = 0; i < population_size; ++i) {
        selection_probabilities[i] = (i + 1) / total_rank;
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    while (selected_parents.size() < num_parents) {
        double random_prob = dis(gen);
        double cumulative_prob = 0.0;
        for (int i = 0; i < population_size; ++i) {
            cumulative_prob += selection_probabilities[i];
            if (random_prob <= cumulative_prob) {
                selected_parents.push_back(population[i]);
                break;
            }
        }
    }
    return selected_parents;
}

void evolve_population(vec_vec_int& old_generation) {
    vec_vec_int parents = rank_based_selection(old_generation);
    vec_vec_int new_generation = old_generation;
    new_generation.erase(new_generation.begin() + elite_size, new_generation.end());
    for(vec_int member : new_generation) {
        int member_fitness = fitness(member);
        //std::cout << member_fitness << '\n';
    }
    for(int i = 1; i < parents.size(); i += 2) {
        vec_vec_int offsprings;
        vec_int parent_A = parents[i - 1];
        vec_int parent_B = parents[i];
        vec_int A_tnerap(parent_A.rbegin(), parent_A.rend());
        vec_int B_tnerap(parent_B.rbegin(), parent_B.rend());
        offsprings.push_back(crossover(parent_A, parent_B));
        offsprings.push_back(crossover(parent_B, parent_A));
        offsprings.push_back(crossover(A_tnerap, parent_B));
        offsprings.push_back(crossover(B_tnerap, parent_A));
        for(vec_int offspring : offsprings) {
            if(is_valid_solution(offspring)) {
                new_generation.push_back(offspring);
            }
        }
    }
    populate(new_generation);
    //mutate(new_generation, stop_vertices_check);
    std::sort(new_generation.begin(), new_generation.end(), [](const vec_int& a, const vec_int& b) {
        return fitness(a) < fitness(b);
    });
    new_generation.erase(new_generation.begin() + population_size, new_generation.end());
    populate(new_generation);
    old_generation = new_generation;
}

void display_population(vec_vec_int population, int np) {
    int max_fitness = INT_MAX;
    std::cout << "Gen " << np + 1 << ": \n";
    for(vec_int member : population) {
        for(int vertex : member) {
            std::cout << vertex << ' ';
        }
        int member_fitness = fitness(member);
        std::cout << "| " << member_fitness <<'\n';
        if(member_fitness < max_fitness) {
            max_fitness = member_fitness;
        }
    }
    //res << max_fitness << '\n'; 
}

void dfs(int n, int v, std::vector<std::vector<int>>& graph, std::vector<bool>& visited) {
    visited[v] = true;
    for (int i = 0; i < n; ++i) {
        if (graph[v][i] != 0 && !visited[i]) {
            dfs(n, i, graph, visited);
        }
    }
}

bool is_connected(int n, std::vector<std::vector<int>> &graph) {
    std::vector<bool> visited(n, 0);
    dfs(n, 0, graph, visited);
    for (bool visit : visited) {
        if (!visit) {
            return 0;
        }
    }
    return 1;
}

void read_data_from_json(std::filesystem::path test_data_path, int &n, int &s, std::vector<std::vector<int>> &graph, std::vector<int> &stop_vertices) {
    std::ifstream file(test_data_path);
    if (file.is_open()) {
        json data;
        file >> data;
        n = data["number of vertices"];
        s = data["number of stop vertices"];
        graph = data["graph"].get<std::vector<std::vector<int>>>();
        stop_vertices = data["stop vertices"].get<std::vector<int>>();
        file.close();
    } else {
        std::cerr << "Unable to open file '" << test_data_path << "'." << '\n';
    }
}

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "needs both paths to data file and to output the solution as argumets\n";
        return -1;
    }
    std::string test_data_path = argv[1];
    read_data_from_json(test_data_path, n, s, graph, stop_vertices);
    if(!is_connected(n, graph)){
        std::cout << -2 << '\n';
        return 0;
    }
    vec_vec_int population;
    stop_vertices_check = vec_bool(n, 0);
    for(auto &v : stop_vertices) {
        stop_vertices_check[v] = 1;
    }
    populate(population);
    for(int i = 0; i < number_of_generations; ++i) {
        evolve_population(population);
    }
    std::cout << fitness(population[0]) << '\n';
    return 0;
}

