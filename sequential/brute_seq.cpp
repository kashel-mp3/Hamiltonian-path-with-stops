/*
Kompilacja:
g++ permutacje3.cpp -o permutacje3

g++ permutacje3.cpp -fopenmp -o permutacje3 

Uruchomienie:
./permutacje3 1


Skrypt:

#!/bin/bash
n1=1
n2=11
step=1
echo "^ \$n\$ ^ \$T_s(n)\$ ^ \$T_r(n)\$ ^ \$T_s(n) \\over T_r(n)\$ ^"
while [ $n1 -le $n2 ]; do
 #echo n1=$n1
 #env OMP_NUM_THREADS=$n1 DRUK=Tak ./permutacje2 $n1
 # env OMP_NUM_THREADS=$n1 DRUK=Nie ./permutacje2 $n1
 ./permutacje3 $n1
 let n1=n1+step
done

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

#include "../utils.h"

typedef std::vector<int> vec_int;
typedef std::vector<bool> vec_bool;
typedef std::vector<std::vector<int>> vec_vec_int;

int** generate_permutations(unsigned int n, uint64_t& count)
{
    count = 1;
    for (unsigned int i = 2; i <= n; i++)
        count *= i;

    int** permutations = new int*[count];
    for (uint64_t i = 0; i < count; i++)
        permutations[i] = new int[n];

    int* perm = new int[n];
    for (unsigned int i = 0; i < n; i++)
        perm[i] = i;

    uint64_t index = 0;
    do
    {
        for (unsigned int i = 0; i < n; i++)
            permutations[index][i] = perm[i];
        index++;
    } while (std::next_permutation(perm, perm + n));

    delete[] perm;
    return permutations;
}

bool is_hamiltonian(int n, int** graph, int* permutation) {

    for (int j = 0; j < n - 1; j++) {
        if (graph[permutation[j]][permutation[j + 1]] == 0) {
            return false;
        }
    }

    return true;
}

int** get_hamiltonian_paths(Utils utils, int n, int** graph, uint64_t& num_paths) {
    std::cout << "Finding Hamiltonian paths...\n";
    if (!utils.is_connected_arrays(n, graph)) {
        std::cout << "Graph is not connected.\n";
        num_paths = 0;
        return nullptr;
    }

    uint64_t count;
    int** permutations = generate_permutations(n, count);

    int** hamiltonian_paths = new int*[count];
    num_paths = 0;

    for (uint64_t i = 0; i < count; i++) {
        if (is_hamiltonian(n, graph, permutations[i])) {
            hamiltonian_paths[num_paths] = new int[n];
            std::copy(permutations[i], permutations[i] + n, hamiltonian_paths[num_paths]);
            num_paths++;
        }
        delete[] permutations[i];
    }

    delete[] permutations;

    if (num_paths > 0) {
        int** resized_paths = new int*[num_paths];
        for (uint64_t i = 0; i < num_paths; i++) {
            resized_paths[i] = hamiltonian_paths[i];
        }
        delete[] hamiltonian_paths;
        hamiltonian_paths = resized_paths;
    }

    return hamiltonian_paths;
}

int max_subpath(int n, int* path, int** graph, bool* stops) {
    int sum = 0;
    int max = 0;
    for (uint64_t i = 0; i < n - 1; ++i) {
        if (stops[path[i]]) {
            if (sum > max)
                max = sum;
            sum = 0;
        }
        sum += graph[path[i]][path[i + 1]];
    }
    if (sum < max)
        max = sum;
    return max;
}

void find_min_max_path(Utils utils, int n, int** graph, bool* stops) {
    uint64_t num_paths;
    int** hamiltonian_paths = get_hamiltonian_paths(utils, n, graph, num_paths);
    int* best_path = nullptr;
    int min_max = INT32_MAX;
    int cur;

    for (uint64_t i = 0; i < num_paths; ++i) {
        cur = max_subpath(n, hamiltonian_paths[i], graph, stops);

        if (cur < min_max) {
            min_max = cur;
            if (best_path) {
                delete[] best_path;
            }
            best_path = new int[n];
            std::copy(hamiltonian_paths[i], hamiltonian_paths[i] + n, best_path);        
            // for (uint64_t i = 0; i < n; i++) {
            //     std::cout << best_path[i] << ' ';
            // }
            // std::cout << min_max << '\n';
        }

        delete[] hamiltonian_paths[i];
    }

    for (uint64_t i = 0; i < n; i++) {
        std::cout << best_path[i] << ' ';
    }
    std::cout << min_max << '\n';

    delete[] hamiltonian_paths;
    delete[] best_path;
}

void print(int n, int** graph, bool* stops) {
    std::cout << "Graph:" << std::endl;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            std::cout << graph[i][j] << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "Stops:" << std::endl;
    for (int i = 0; i < n; i++) {
        std::cout << stops[i] << " ";
    }
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    Utils utils = Utils();
    // if(argc < 2) {
    //     std::cerr << "path to data file not provided\n";
    //     return 1;
    // }
    //std::string test_data_path = argv[1];
    std::string  test_data_path = "tests/n_p=50/n_5_p_50_s_2.json";
    int n, s;
    bool* stop_vertices;
    int** graph;
    utils.read_data_from_json_to_arrays(test_data_path, n, s, graph, stop_vertices);

    find_min_max_path(utils, n, graph, stop_vertices);
    //print(n, graph, stop_vertices);
    // if(!utils.is_connected_arrays(n, graph)) {
    //     std::cout << "not connected \n";
    //     return 0;
    // } 
//     uint64_t L1 = 0;
//     uint64_t L2 = 0;

//     std::chrono::time_point<std::chrono::system_clock> start, koniec;

//     start = std::chrono::system_clock::now();
//     L1 = f_sec(n);
//     koniec = std::chrono::system_clock::now();
//     std::chrono::duration<double> czas_wykonania_sec = koniec - start;

//     start = std::chrono::system_clock::now();
//     L2 = f_par(n);
//     koniec = std::chrono::system_clock::now();

//     std::chrono::duration<double> czas_wykonania_par = koniec - start;
//     std::clog << std::fixed << std::setprecision(7);
//     std::clog << "| " << n << " | ";
//     std::clog << czas_wykonania_sec.count() << " | ";
//     std::clog << czas_wykonania_par.count() << " | ";
//     std::clog << czas_wykonania_sec.count() / czas_wykonania_par.count() << " | " << std::endl;

// #ifdef _OPENMP
//     assert(L1 == L2);
// #endif
    utils.release_allocated_memory(n, graph, stop_vertices);
    return EXIT_SUCCESS;
}