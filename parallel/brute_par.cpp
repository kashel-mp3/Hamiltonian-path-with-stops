#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>
#include <omp.h>

#include "../utils.h"

std::vector<uint64_t> calculate_factorials(unsigned int n) {
    std::vector<uint64_t> facts(n + 1);
    facts[0] = 1;
    for (unsigned int i = 1; i <= n; ++i) {
        
        if (i > 0 && facts[i - 1] > std::numeric_limits<uint64_t>::max() / i) {
             throw std::overflow_error("Factorial calculation overflows uint64_t for n=" + std::to_string(i));
        }
        facts[i] = facts[i - 1] * i;
    }
    return facts;
}

void get_kth_permutation(unsigned int n, uint64_t k, const std::vector<uint64_t>& factorials, int* result_perm) {
    std::vector<int> available_elements(n);
    std::iota(available_elements.begin(), available_elements.end(), 0);

    uint64_t remainder = k;

    for (unsigned int i = 0; i < n; ++i) {
        unsigned int factorial_idx = n - 1 - i;
        uint64_t factorial_val = (factorial_idx < factorials.size()) ? factorials[factorial_idx] : 1;
        if (factorial_val == 0) factorial_val = 1; 

        uint64_t index_in_available = remainder / factorial_val;
        remainder = remainder % factorial_val;

        if (index_in_available >= available_elements.size()) {
            
             index_in_available = available_elements.size() -1; 
        }

        result_perm[i] = available_elements[index_in_available];
        available_elements.erase(available_elements.begin() + index_in_available);
    }
}

bool is_hamiltonian(int n, int **graph, const int *permutation, const bool *stops)
{
    if (!stops[permutation[0]] || !stops[permutation[n - 1]]) return false;
    for (int j = 0; j < n - 1; j++)
    {
        if (graph[permutation[j]][permutation[j + 1]] == 0)
        {
            return false;
        }
    }
    return true;
}

int max_subpath(int n, const int *path, int **graph, const bool *stops)
{
    int current_subpath_sum = 0;
    int max_found = 0; 

    for (int i = 0; i < n - 1; ++i)
    {
        
        if (stops[path[i]])
        {
            
            if (current_subpath_sum > max_found) {
                max_found = current_subpath_sum;
            }
            
            current_subpath_sum = 0;
        }

        int edge_weight = graph[path[i]][path[i + 1]];
        if (edge_weight == 0) {
            
        }
        current_subpath_sum += edge_weight;
    }

    if (current_subpath_sum > max_found) {
        max_found = current_subpath_sum;
    }

    return max_found;
}

void find_min_max_path(Utils utils, int n, int **graph, bool *stops)
{
    if (n <= 1) {
        std::cout << "Path requires at least 2 vertices.\n";
        return;
    }

    if (!utils.is_connected_arrays(n, graph))
    {
        std::cout << "Graph is not connected. No Hamiltonian path possible.\n";
        return;
    }

    std::vector<uint64_t> factorials;
    uint64_t permutation_count = 0;
    try {
        factorials = calculate_factorials(n);
        permutation_count = factorials[n];
         if (n > 0 && permutation_count == 0) { 
             throw std::overflow_error("n! count exceeds uint64_t capacity.");
         }
    } catch (const std::overflow_error& e) {
        std::cerr << "Error during precomputation: " << e.what() << std::endl;
        return;
    }
    std::cout << "Total permutations to check: " << permutation_count << std::endl;

    std::vector<int> global_best_path;
    int global_min_max = std::numeric_limits<int>::max();

    #pragma omp parallel
    {
        std::vector<int> local_best_path(n);
        int local_min_max = std::numeric_limits<int>::max();
        bool thread_found_path = false;

        std::vector<int> current_perm_buffer(n);
        const int chunk_size = 10000;

        #pragma omp for schedule(dynamic, chunk_size) nowait 
        for (uint64_t k = 0; k < permutation_count; ++k) {

            get_kth_permutation(n, k, factorials, current_perm_buffer.data());

            if (is_hamiltonian(n, graph, current_perm_buffer.data(), stops)) {

                int current_max_subpath = max_subpath(n, current_perm_buffer.data(), graph, stops);

                if (current_max_subpath < local_min_max) {
                    local_min_max = current_max_subpath;
                    
                    local_best_path = current_perm_buffer; 
                    thread_found_path = true;
                }
            }
        } 

        #pragma omp critical
        {
            if (thread_found_path && local_min_max < global_min_max) {
                std::cout << "Thread " << omp_get_thread_num()
                          << " found new best path with max subpath: " << local_min_max
                          << " (beating global " << global_min_max << ")" << std::endl; 
                global_min_max = local_min_max;
                global_best_path = local_best_path; 
            }
        } 

    } 

    if (global_best_path.empty()) {
        std::cout << "No Hamiltonian path found that satisfies the conditions.\n";
    } else {
        for (int node : global_best_path) {
            std::cout << node << ' ';
        }
        std::cout << global_min_max << '\n';
    }

}

int main(int argc, char *argv[])
{
    Utils utils = Utils();
    if (argc < 2)
    {
        std::cerr << "path to data file not provided\n";
        return 1;
    }
    std::string test_data_path = argv[1];
    int n, s;
    int *stop_vertices;
    int **graph;
    utils.read_data_from_json_to_arrays(test_data_path, n, s, graph, stop_vertices);

    bool *stop_vertices_check = new bool[n]();
    for (int i = 0; i < s; ++i) {
        stop_vertices_check[stop_vertices[i]] = true;
    }

    find_min_max_path(utils, n, graph, stop_vertices_check);

    delete[] stop_vertices_check;
    utils.release_allocated_memory(n, graph, stop_vertices);
    return EXIT_SUCCESS;
}