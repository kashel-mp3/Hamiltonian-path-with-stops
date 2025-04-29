#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <omp.h>
#include <numeric>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include "utils.h"

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


void generate_permutations(unsigned int n, const std::function<void(const int*)>& callback)
{
    int* perm = new int[n];
    for (unsigned int i = 0; i < n; i++)
        perm[i] = i;

    do
    {
        callback(perm);
    } while (std::next_permutation(perm, perm + n));

    delete[] perm;
}

// Include your existing utility functions here (e.g., calculate_factorials, get_kth_permutation, generate_permutations)

void benchmark_permutations(std::string& test_case_path, const std::string& report_path) {
    // Read test case data
    Utils utils;
    int n, s;
    bool* stop_vertices;
    int** graph;
    utils.read_data_from_json_to_arrays(test_case_path, n, s, graph, stop_vertices);

    // Precompute factorials
    std::vector<uint64_t> factorials = calculate_factorials(n);

    // Benchmark generate_permutations
    auto start = std::chrono::high_resolution_clock::now();
    generate_permutations(n, [](const int* perm) {
        // Do nothing, just iterate through permutations
    });
    auto end = std::chrono::high_resolution_clock::now();
    double time_generate_permutations = std::chrono::duration<double>(end - start).count();
    if (time_generate_permutations > 60) time_generate_permutations = 60;

    // Benchmark get_kth_permutation (sequential)
    start = std::chrono::high_resolution_clock::now();
    std::vector<int> current_perm_buffer(n);
    uint64_t permutation_count = factorials[n];
    for (uint64_t k = 0; k < permutation_count; ++k) {
        get_kth_permutation(n, k, factorials, current_perm_buffer.data());
        if (std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() > 60) {
            time_get_kth_permutation = 60;
            break;
        }
    }
    end = std::chrono::high_resolution_clock::now();
    double time_get_kth_permutation = std::chrono::duration<double>(end - start).count();
    if (time_get_kth_permutation > 60) time_get_kth_permutation = 60;

    // Benchmark get_kth_permutation with OpenMP (dynamic scheduling)
    double time_ktm_static = 0.0, time_ktm_dynamic = 0.0, time_ktm_dynamic_chunk_1000 = 0.0, time_ktm_dynamic_chunk_10000 = 0.0, time_ktm_dynamic_chunk_100000 = 0.0;

    // Static scheduling
    start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel
    {
        #pragma omp for schedule(static) nowait
        for (uint64_t k = 0; k < permutation_count; ++k) {
            get_kth_permutation(n, k, factorials, current_perm_buffer.data());
            if (std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() > 60) {
                #pragma omp critical
                time_ktm_static = 60;
                break;
            }
        }
    }
    end = std::chrono::high_resolution_clock::now();
    if (time_ktm_static != 60) time_ktm_static = std::chrono::duration<double>(end - start).count();

    // Dynamic scheduling
    start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel
    {
        #pragma omp for schedule(dynamic) nowait
        for (uint64_t k = 0; k < permutation_count; ++k) {
            get_kth_permutation(n, k, factorials, current_perm_buffer.data());
            if (std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() > 60) {
                #pragma omp critical
                time_ktm_dynamic = 60;
                break;
            }
        }
    }
    end = std::chrono::high_resolution_clock::now();
    if (time_ktm_dynamic != 60) time_ktm_dynamic = std::chrono::duration<double>(end - start).count();

    // Dynamic scheduling with chunk size 1000
    start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel
    {
        #pragma omp for schedule(dynamic, 1000) nowait
        for (uint64_t k = 0; k < permutation_count; ++k) {
            get_kth_permutation(n, k, factorials, current_perm_buffer.data());
            if (std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() > 60) {
                #pragma omp critical
                time_ktm_dynamic_chunk_1000 = 60;
                break;
            }
        }
    }
    end = std::chrono::high_resolution_clock::now();
    if (time_ktm_dynamic_chunk_1000 != 60) time_ktm_dynamic_chunk_1000 = std::chrono::duration<double>(end - start).count();

    // Dynamic scheduling with chunk size 10000
    start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel
    {
        #pragma omp for schedule(dynamic, 10000) nowait
        for (uint64_t k = 0; k < permutation_count; ++k) {
            get_kth_permutation(n, k, factorials, current_perm_buffer.data());
            if (std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() > 60) {
                #pragma omp critical
                time_ktm_dynamic_chunk_10000 = 60;
                break;
            }
        }
    }
    end = std::chrono::high_resolution_clock::now();
    if (time_ktm_dynamic_chunk_10000 != 60) time_ktm_dynamic_chunk_10000 = std::chrono::duration<double>(end - start).count();

    // Dynamic scheduling with chunk size 100000
    start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel
    {
        #pragma omp for schedule(dynamic, 100000) nowait
        for (uint64_t k = 0; k < permutation_count; ++k) {
            get_kth_permutation(n, k, factorials, current_perm_buffer.data());
            if (std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() > 60) {
                #pragma omp critical
                time_ktm_dynamic_chunk_100000 = 60;
                break;
            }
        }
    }
    end = std::chrono::high_resolution_clock::now();
    if (time_ktm_dynamic_chunk_100000 != 60) time_ktm_dynamic_chunk_100000 = std::chrono::duration<double>(end - start).count();


    // Write results to report file
    std::ofstream report_file(report_path, std::ios::app);
    if (report_file.is_open()) {
        report_file << test_case_path << " "
                    << time_generate_permutations << " "
                    << time_get_kth_permutation << " "
                    << time_ktm_static << " "
                    << time_ktm_dynamic << " "
                    << time_ktm_dynamic_chunk_1000 << " "
                    << time_ktm_dynamic_chunk_10000 << " "
                    << time_ktm_dynamic_chunk_100000 << "\n";
        report_file.close();
    } else {
        std::cerr << "Failed to open report file: " << report_path << "\n";
    }

    // Print results to console in one line with equal distance
    std::cout << std::left << std::setw(10) << "Test case: " << test_case_path << "  "
              << std::setw(20) << time_generate_permutations
              << std::setw(20) << time_get_kth_permutation
              << std::setw(20) << time_ktm_static
              << std::setw(20) << time_ktm_dynamic
              << std::setw(20) << time_ktm_dynamic_chunk_1000
              << std::setw(20)  << time_ktm_dynamic_chunk_10000
              << std::setw(20)  << time_ktm_dynamic_chunk_100000
              << "\n";

    // Release allocated memory
    utils.release_allocated_memory(n, graph, stop_vertices);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <relative_path_to_test_cases>\n";
        return 1;
    }

    std::string test_cases_path = argv[1];
    std::string report_dir = "raports/perm/";
    std::filesystem::create_directories(report_dir);

    std::cout << std::left << std::setw(43) << "Test case name "
              << std::setw(20) << "permutations"
              << std::setw(20) << "kth_permutation"
              << std::setw(20) << "ktm_static"
              << std::setw(20) << "ktm_dynamic"
              << std::setw(20) << "chunk_1000"
              << std::setw(20)  << "chunk_10000"
              << std::setw(20)  << "chunk_100000"
              << "\n";

    for (const auto& entry : std::filesystem::directory_iterator(test_cases_path)) {
        if (entry.is_regular_file()) {
            std::string test_case_path = entry.path().string();
            std::string report_path = report_dir + "benchmark_results.txt";
            benchmark_permutations(test_case_path, report_path);
        }
    }

    return 0;
}
