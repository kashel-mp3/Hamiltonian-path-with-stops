#include <vector>
#include <numeric>   // For std::iota
#include <algorithm> // For std::next_permutation, std::sort, std::copy
#include <cstdint>   // For uint64_t
#include <omp.h>     // For OpenMP
#include <stdexcept> // For std::overflow_error
#include <limits>    // For numeric_limits
#include <iostream>  // For output/debugging
#include <chrono>

// --- Your Processing Function Placeholder ---
// This function will be called for ~n!/2 valid permutations.
// It needs to be thread-safe if it modifies shared data.
// Add necessary parameters (graph, stops, thread-local results etc.)
void process_permutation_task(const int* perm, int n /*, Args... */) {
    // Example: check hamiltonian, calculate max_subpath, compare to thread-local best
    // std::cout << "Thread " << omp_get_thread_num() << " processing: ";
    // for(int i=0; i<n; ++i) std::cout << perm[i] << " ";
    // std::cout << std::endl;

    // IMPORTANT: If finding a single best result overall (like min_max_path),
    // you'll need thread-local storage for the best result found *by that thread*
    // and a critical section after the parallel loop to merge thread results.
}

// --- Main Function using Parallel Prefix Decomposition ---
void generate_and_process_parallel_prefix(int n /*, Args needed by process_permutation_task... */) {

    if (n <= 1) {
        std::cout << "N must be >= 2 for permutations.\n";
        return;
    }

    // --- Threshold for Sequential Execution ---
    const int sequential_threshold = 6; // Run sequentially for n < 6

    if (n < sequential_threshold) {
        std::vector<int> perm(n);
        std::iota(perm.begin(), perm.end(), 0); // Initial permutation 0, 1, ..., n-1

        uint64_t count = 0;
        do {
            // Check canonical form for bidirectional paths before processing
            if (perm[0] < perm[n - 1]) {
                process_permutation_task(perm.data(), n /*, Args... */);
                count++;
            }
        } while (std::next_permutation(perm.begin(), perm.end()));
         std::cout << "[Info] Sequentially processed " << count << " canonical permutations." << std::endl;

    } else {
        // --- Parallel Execution for n >= threshold ---
        uint64_t parallel_processed_count = 0; // Use reduction for accurate counting

        #pragma omp parallel reduction(+:parallel_processed_count)
        {
            // --- Thread-Local Storage ---
            // Buffer for the current permutation being worked on by this thread
            std::vector<int> local_perm(n);

            // Add any other thread-local variables needed for process_permutation_task
            // e.g., int thread_local_best_val = std::numeric_limits<int>::max();
            // e.g., std::vector<int> thread_local_best_path(n);


            // Distribute the n*(n-1) prefix tasks statically
            #pragma omp for collapse(2) schedule(static) nowait
            for (int i = 0; i < n; ++i) { // First element of prefix (p0)
                for (int j = 0; j < n; ++j) { // Second element of prefix (p1)

                    if (i == j) continue; // Skip prefixes like (i, i)

                    // --- Setup local_perm for prefix (i, j) ---
                    local_perm[0] = i;
                    local_perm[1] = j;

                    // Fill the rest [2...n-1] with remaining n-2 numbers, sorted.
                    int current_fill_val = 0;
                    int suffix_idx = 2;
                    while (suffix_idx < n) {
                        // Find next value not equal to i or j
                        while (current_fill_val == i || current_fill_val == j) {
                            current_fill_val++;
                        }
                        local_perm[suffix_idx++] = current_fill_val++;
                    }
                    // Suffix local_perm[2...n-1] is now sorted

                    // --- Process the first permutation for this prefix ---
                    // Check canonical form (p0 < pn-1)
                    if (local_perm[0] < local_perm[n - 1]) {
                        process_permutation_task(local_perm.data(), n /*, Args... */);
                        parallel_processed_count++;
                    }


                    // --- Generate and process subsequent permutations for this prefix ---
                    if (n > 2) // Only permute suffix if its length > 0
                    {
                         while (std::next_permutation(local_perm.begin() + 2, local_perm.end()))
                         {
                             // Check canonical form (p0 < pn-1) before processing
                            if (local_perm[0] < local_perm[n - 1]) {
                                process_permutation_task(local_perm.data(), n /*, Args... */);
                                parallel_processed_count++;
                            }
                         }
                         // Only permute the suffix [2...n-1]
                    }
                } // end loop j (prefix element 1)
            } // end loop i (prefix element 0)
            // End of omp for loop. "nowait" allows threads to proceed.

            // --- Merge thread-local results (if necessary) ---
            // If process_permutation_task updated thread-local bests, merge them here
            // #pragma omp critical
            // {
            //     // Compare thread_local_best_val with global_best_val
            //     // Update global_best_val and global_best_path if needed
            // }

        } // --- End of parallel region ---

        std::cout << "[Info] Parallel processed " << parallel_processed_count << " canonical permutations." << std::endl;
    } // end if n >= threshold
} // end function

int main(int argc, char* argv[]) {
    if (argc = 0)
        return 1;
    int n = atoi(argv[1]); 
    auto start = std::chrono::high_resolution_clock::now();
    generate_and_process_parallel_prefix(n);
    auto end = std::chrono::high_resolution_clock::now();
    double time_generate_permutations = std::chrono::duration<double>(end - start).count();
    std::cout << time_generate_permutations << '\n';
    return 0;
}