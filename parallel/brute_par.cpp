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

typedef std::vector<int> vec_int;
typedef std::vector<bool> vec_bool;
typedef std::vector<std::vector<int>> vec_vec_int;

int **generate_permutations(unsigned int n, uint64_t &count)
{
    count = 1;
    for (unsigned int i = 2; i <= n; i++)
        count *= i;

    int **permutations = new int *[count];
    for (uint64_t i = 0; i < count; i++)
        permutations[i] = new int[n];

    int *perm = new int[n];
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

bool is_hamiltonian(int n, int **graph, int *permutation)
{

    for (int j = 0; j < n - 1; j++)
    {
        if (graph[permutation[j]][permutation[j + 1]] == 0)
        {
            return false;
        }
    }

    return true;
}

int **get_hamiltonian_paths(Utils utils, int n, int **graph, uint64_t &num_paths)
{
    if (!utils.is_connected_arrays(n, graph))
    {
        std::cout << "Graph is not connected.\n";
        num_paths = 0;
        return nullptr;
    }

    uint64_t count;
    int **permutations = generate_permutations(n, count);

    int **hamiltonian_paths = new int *[count];
    uint64_t *local_counts = new uint64_t[omp_get_max_threads()]();

    int ***local_paths = new int **[omp_get_max_threads()];
    for (int i = 0; i < omp_get_max_threads(); ++i)
    {
        local_paths[i] = new int *[count];
    }

#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        uint64_t local_num = 0;

#pragma omp for schedule(dynamic)
        for (uint64_t i = 0; i < count; i++)
        {
            if (is_hamiltonian(n, graph, permutations[i]))
            {
                int *temp = new int[n];
                std::copy(permutations[i], permutations[i] + n, temp);
                local_paths[tid][local_num++] = temp;
            }
            delete[] permutations[i];
        }
        local_counts[tid] = local_num;
    }

    delete[] permutations;

    num_paths = 0;
    for (int t = 0; t < omp_get_max_threads(); ++t)
    {
        for (uint64_t j = 0; j < local_counts[t]; ++j)
        {
            hamiltonian_paths[num_paths++] = local_paths[t][j];
        }
        delete[] local_paths[t];
    }
    delete[] local_paths;
    delete[] local_counts;

    return hamiltonian_paths;
}

int max_subpath(int n, int *path, int **graph, bool *stops)
{
    int sum = 0;
    int max = 0;
    for (uint64_t i = 0; i < n - 1; ++i)
    {
        if (stops[path[i]])
        {
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

void find_min_max_path(Utils utils, int n, int **graph, bool *stops)
{
    uint64_t num_paths;
    int **hamiltonian_paths = get_hamiltonian_paths(utils, n, graph, num_paths);
    int *best_path = nullptr;
    int min_max = INT32_MAX;
    int cur;

#pragma omp parallel
    {
        int *local_best_path = new int[n];
        int local_min_max = INT32_MAX;

#pragma omp for nowait
        for (uint64_t i = 0; i < num_paths; ++i)
        {
            int cur = max_subpath(n, hamiltonian_paths[i], graph, stops);
            if (cur < local_min_max)
            {
                local_min_max = cur;
                std::copy(hamiltonian_paths[i], hamiltonian_paths[i] + n, local_best_path);
            }
        }

#pragma omp critical
        {
            if (local_min_max < min_max)
            {
                min_max = local_min_max;
                if (best_path)
                    delete[] best_path;
                best_path = new int[n];
                std::copy(local_best_path, local_best_path + n, best_path);
            }
        }

        delete[] local_best_path;
    }

    for (uint64_t i = 0; i < n; i++)
    {
        std::cout << best_path[i] << ' ';
    }
    std::cout << min_max << '\n';

    delete[] hamiltonian_paths;
    delete[] best_path;
}

uint64_t f_par(unsigned int n)
{
    uint64_t L = 0;
#pragma omp parallel reduction(+ : L) num_threads(n)
    {
        int *perm = new int[n];
        int tid = omp_get_thread_num();
        perm[0] = tid;
        for (unsigned int i = 1; i <= tid; i++)
            perm[i] = i - 1;
        for (unsigned int i = tid + 1; i < n; i++)
            perm[i] = i;
        do
        {
            L++;
        } while (std::next_permutation(perm + 1, perm + n));
        delete perm;
    }
    return L;
}

void print(int n, int **graph, bool *stops)
{
    std::cout << "Graph:" << std::endl;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            std::cout << graph[i][j] << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "Stops:" << std::endl;
    for (int i = 0; i < n; i++)
    {
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
    // std::string test_data_path = argv[1];
    std::string test_data_path = "tests/n_p=50/n_5_p_50_s_2.json";
    int n, s;
    bool *stop_vertices;
    int **graph;
    utils.read_data_from_json_to_arrays(test_data_path, n, s, graph, stop_vertices);

    find_min_max_path(utils, n, graph, stop_vertices);
    // print(n, graph, stop_vertices);
    //  if(!utils.is_connected_arrays(n, graph)) {
    //      std::cout << "not connected \n";
    //      return 0;
    //  }
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