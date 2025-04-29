/*
Kompilacja:
g++ permutacje3.cpp -o permutacje3

g++ permutacje3.cpp -fopenmp -o permutacje3

Uruchomienie:
./permutacje3 1
*/

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

// Helper function to calculate factorials (can overflow for n > 20)
// check if only half the permutations can be generated
std::vector<uint64_t> calculate_factorials(unsigned int n) {
    std::vector<uint64_t> facts(n + 1);
    facts[0] = 1;
    for (unsigned int i = 1; i <= n; ++i) {
        // Check for potential overflow before multiplication
        if (i > 0 && facts[i - 1] > std::numeric_limits<uint64_t>::max() / i) {
             throw std::overflow_error("Factorial calculation overflows uint64_t for n=" + std::to_string(i));
        }
        facts[i] = facts[i - 1] * i;
    }
    return facts;
}

// Function to generate the k-th lexicographical permutation of 0..n-1
// Uses precomputed factorials
// Writes the result into the provided result_perm array/vector data
void get_kth_permutation(unsigned int n, uint64_t k, const std::vector<uint64_t>& factorials, int* result_perm) {
    std::vector<int> available_elements(n);
    std::iota(available_elements.begin(), available_elements.end(), 0);

    uint64_t remainder = k;

    for (unsigned int i = 0; i < n; ++i) {
        unsigned int factorial_idx = n - 1 - i;
        uint64_t factorial_val = (factorial_idx < factorials.size()) ? factorials[factorial_idx] : 1;
        if (factorial_val == 0) factorial_val = 1; // Avoid division by zero for 0!

        uint64_t index_in_available = remainder / factorial_val;
        remainder = remainder % factorial_val;

        if (index_in_available >= available_elements.size()) {
            // Should not happen if k < n!
            // Handle defensively - place last element? Or throw?
             // This might indicate k was too large or calculation error.
             // For robustness maybe: index_in_available = available_elements.size() - 1;
             // Or better: throw std::runtime_error("Permutation index calculation error.");
             index_in_available = available_elements.size() -1; // Simple clamp for now
        }

        result_perm[i] = available_elements[index_in_available];
        available_elements.erase(available_elements.begin() + index_in_available);
    }
}

bool is_hamiltonian(int n, int **graph, const int *permutation)
{
    for (int j = 0; j < n - 1; j++)
    {
        if (graph[permutation[j]][permutation[j + 1]] == 0)
        {
            return false;
        }
    }
    // if (graph[permutation[n - 1]][permutation[0]] == 0) return false;

    return true;
}

int max_subpath(int n, const int *path, int **graph, const bool *stops)
{
    int current_subpath_sum = 0;
    int max_found = 0; // Initialize max found subpath sum

    for (int i = 0; i < n - 1; ++i)
    {
        // If the current node is a designated stop
        if (stops[path[i]])
        {
            // Check if the completed subpath is the longest so far
            if (current_subpath_sum > max_found) {
                max_found = current_subpath_sum;
            }
            // Reset sum for the next subpath starting after this stop
            current_subpath_sum = 0;
        }

        // Add the weight of the edge from path[i] to path[i+1]
        // Check if edge exists? is_hamiltonian should guarantee this, but belt-and-suspenders?
        int edge_weight = graph[path[i]][path[i + 1]];
        if (edge_weight == 0) {
            // This path isn't actually valid according to the graph weights provided
            // Handle error: return a special value (e.g., -1) or throw?
            // Let's assume is_hamiltonian already validated this.
            // std::cerr << "Warning: Edge weight 0 found in supposedly Hamiltonian path.\n";
        }
        current_subpath_sum += edge_weight;
    }

    // After the loop, check the last subpath (from the last stop to the end)
    if (current_subpath_sum > max_found) {
        max_found = current_subpath_sum;
    }

    // If the path had no stops at all, max_found might still be 0 if all weights are 0.
    // If the path has stops, but all segments sum to 0 or less, max_found will be 0.
    // If no edges were traversed (n=1), max_found is 0.
    // Consider edge cases based on problem definition if stops[path[0]] should reset sum immediately.
    // The current logic calculates sums *between* stops (and start/end).

    return max_found;
}


// --- Refactored Main Function ---

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

        #pragma omp for schedule(dynamic, chunk_size) nowait // nowait: threads proceed to critical section when done
        for (uint64_t k = 0; k < permutation_count; ++k) {

            // 1. Generate k-th permutation into thread-local buffer
            get_kth_permutation(n, k, factorials, current_perm_buffer.data());

            // 2. Check if it's a Hamiltonian path
            if (is_hamiltonian(n, graph, current_perm_buffer.data())) {

                // 3. Calculate the max subpath length for this valid path
                int current_max_subpath = max_subpath(n, current_perm_buffer.data(), graph, stops);

                // 4. Check if this path is better than the thread's current best
                if (current_max_subpath < local_min_max) {
                    local_min_max = current_max_subpath;
                    // Copy the current permutation to the thread's best path buffer
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

        // local_best_path, current_perm_buffer go out of scope and are cleaned up here
        // thread_found_path, local_min_max also go out of scope

    } // End of parallel region


    // --- Output Results (Sequential) ---
    if (global_best_path.empty()) {
        std::cout << "No Hamiltonian path found that satisfies the conditions.\n";
    } else {
        for (int node : global_best_path) {
            std::cout << node << ' ';
        }
        std::cout << global_min_max << '\n';
    }

    // No need to delete hamiltonian_paths array as it was never created
    // global_best_path is a std::vector and cleans itself up
}

// void print(int n, int **graph, bool *stops)
// {
//     std::cout << "Graph:" << std::endl;
//     for (int i = 0; i < n; i++)
//     {
//         for (int j = 0; j < n; j++)
//         {
//             std::cout << graph[i][j] << " ";
//         }
//         std::cout << std::endl;
//     }

//     std::cout << "Stops:" << std::endl;
//     for (int i = 0; i < n; i++)
//     {
//         std::cout << stops[i] << " ";
//     }
//     std::cout << std::endl;
// }

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
    bool *stop_vertices;
    int **graph;
    utils.read_data_from_json_to_arrays(test_data_path, n, s, graph, stop_vertices);

    find_min_max_path(utils, n, graph, stop_vertices);

    utils.release_allocated_memory(n, graph, stop_vertices);
    return EXIT_SUCCESS;
}