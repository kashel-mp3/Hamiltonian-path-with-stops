#include <vector>
#include <numeric>   // Required for std::iota
#include <algorithm> // Required for std::next_permutation, std::copy
#include <iostream>
#include <limits>    // Required for std::numeric_limits
#include <string>    // Required for std::string
#include <cstdint>   // Required for uint64_t
// Assuming utils.h contains necessary definitions like Utils class,
// read_data_from_json_to_arrays, release_allocated_memory, is_connected_arrays
#include "../utils.h" 

// --- Type Aliases (optional, kept for consistency) ---
typedef std::vector<int> vec_int;
typedef std::vector<bool> vec_bool;
typedef std::vector<std::vector<int>> vec_vec_int;

// --- Core Logic Functions (kept from original sequential or slightly adapted) ---

// Checks if a given permutation represents a valid Hamiltonian path in the graph.
// No changes needed from the original sequential version.
bool is_hamiltonian(int n, int** graph, const int* permutation) {
    // Check edges between consecutive vertices in the permutation
    for (int j = 0; j < n - 1; j++) {
        if (graph[permutation[j]][permutation[j + 1]] == 0) {
            return false; // Edge doesn't exist
        }
    }

    // Optional: Check edge from last back to first if it's a Hamiltonian *cycle* problem.
    // The original code didn't check this, implying path, so we'll stick to that.
    // if (n > 0 && graph[permutation[n - 1]][permutation[0]] == 0) {
    //     return false; // Edge back to start doesn't exist
    // }

    return true; // All required edges exist
}

// Calculates the maximum weight of a subpath between designated 'stops'.
// Using the version from the parallel code which seems slightly clearer.
// Const correctness added for parameters that are not modified.
int max_subpath(int n, const int* path, int** graph, const bool* stops) {
    int current_subpath_sum = 0;
    int max_found = 0; // Initialize max found subpath sum

    // Handle edge case of very small n
    if (n < 2) {
        return 0;
    }

    for (int i = 0; i < n - 1; ++i) {
        // If the current node is a designated stop, reset the subpath sum *after* potentially updating max.
        // The logic calculates the sum of segments *ending* at a stop (or the end of the path).
        if (stops[path[i]]) {
            if (current_subpath_sum > max_found) {
                max_found = current_subpath_sum;
            }
            current_subpath_sum = 0; // Reset for the next segment starting from this stop
        }

        // Add the weight of the edge from path[i] to path[i+1]
        // is_hamiltonian should have ensured this edge exists and weight > 0 (if weights represent existence)
        // If graph stores weights directly, this is correct.
        int edge_weight = graph[path[i]][path[i + 1]];
        // Assuming weight 0 means no edge was handled by is_hamiltonian. If 0 is a valid weight, this is fine.
        current_subpath_sum += edge_weight;
    }

    // After the loop, check the last subpath (from the last stop, or start, to the end)
    if (current_subpath_sum > max_found) {
        max_found = current_subpath_sum;
    }

    return max_found;
}


// --- Refactored Main Processing Function ---

// Finds the Hamiltonian path with the minimum maximum subpath length.
// This function now incorporates the permutation generation loop.
void find_min_max_path_sequential_optimized(Utils utils, int n, int** graph, bool* stops) {
    std::cout << "Finding Hamiltonian path with minimum maximum subpath (sequential, memory optimized)...\n";

    // Basic checks
    if (n <= 1) {
        std::cout << "Path requires at least 2 vertices.\n";
        return;
    }

    if (!utils.is_connected_arrays(n, graph)) {
        std::cout << "Graph is not connected. No Hamiltonian path possible.\n";
        return;
    }

    // --- Initialization ---
    std::vector<int> current_perm(n);
    std::iota(current_perm.begin(), current_perm.end(), 0); // Initialize permutation to 0, 1, ..., n-1

    std::vector<int> best_path; // Stores the best path found so far
    int min_max_found = std::numeric_limits<int>::max(); // Initialize with maximum possible value
    bool found_any_hamiltonian = false;

    // --- Permutation Generation and Processing Loop ---
    uint64_t permutations_checked = 0; // Counter (optional)
    do {
        permutations_checked++;

        // 1. Check if the current permutation is a Hamiltonian path
        if (is_hamiltonian(n, graph, current_perm.data())) {

            // 2. Calculate the max subpath length for this valid path
            int current_max_subpath = max_subpath(n, current_perm.data(), graph, stops);

            // 3. Check if this path is better than the current best
            if (!found_any_hamiltonian || current_max_subpath < min_max_found) {
                min_max_found = current_max_subpath;
                best_path = current_perm; // Copy the current permutation
                found_any_hamiltonian = true;
                
                // Optional: Print progress
                // std::cout << "Found new best: max_subpath=" << min_max_found << " for path: ";
                // for(int node : best_path) std::cout << node << " ";
                // std::cout << std::endl;
            }
        }

        // 4. Generate the next permutation lexicographically.
        // The current permutation `current_perm` is modified in place.
        // It is implicitly "discarded" in the sense that we don't store it unless it's the best one so far.
    } while (std::next_permutation(current_perm.begin(), current_perm.end()));

    std::cout << "Checked " << permutations_checked << " permutations.\n";

    // --- Output Results ---
    if (!found_any_hamiltonian) {
        std::cout << "No Hamiltonian path found in the graph.\n";
    } else {
        std::cout << "Best path found: ";
        for (size_t i = 0; i < best_path.size(); ++i) {
            std::cout << best_path[i] << (i == best_path.size() - 1 ? "" : " ");
        }
        std::cout << "\nMinimum maximum subpath length: " << min_max_found << '\n';
    }

    // No manual deallocation needed for std::vector 'current_perm' and 'best_path'
}


// --- Main Function (mostly unchanged) ---
int main(int argc, char *argv[])
{
    Utils utils = Utils(); // Assuming Utils has a default constructor
    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_data_file.json>\n";
        return 1;
    }
    std::string test_data_path = argv[1];
    int n = 0;
    int s = 0; // Assuming 's' was number of stops, might not be needed directly by find_min_max_path
    bool* stop_vertices = nullptr;
    int** graph = nullptr;

    try {
        utils.read_data_from_json_to_arrays(test_data_path, n, s, graph, stop_vertices);
        
        // Call the refactored function
        find_min_max_path_sequential_optimized(utils, n, graph, stop_vertices);

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
         // Ensure cleanup even if reading fails or processing throws
        if (graph) utils.release_allocated_memory(n, graph, stop_vertices); // Pass potentially uninitialized n if read failed early
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        if (graph) utils.release_allocated_memory(n, graph, stop_vertices);
        return 1;
    }


    // Clean up allocated memory
    utils.release_allocated_memory(n, graph, stop_vertices);
    
    return EXIT_SUCCESS;
}