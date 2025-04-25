#include <nlohmann/json.hpp>
#include "../utils.h" // Ensure the relative path is correct or adjust it to the actual location of utils.h
#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <climits>
#include <omp.h> // Ensure OpenMP is installed and properly configured in your build system

using json = nlohmann::json;

typedef std::vector<int> vec_int;
typedef std::vector<bool> vec_bool;
typedef std::vector<std::vector<int>> vec_vec_int;

struct PathWithMaxLength
{
    std::vector<int> path;
    int max_length;
};

struct subpath
{
    vec_int path;
    int weight;
};

PathWithMaxLength solve(int n, int s, const vec_vec_int &graph_original, const vec_int &all_stop_vertices)
{
    PathWithMaxLength best_result = {{}, INT_MAX};

#pragma omp parallel
    {
        PathWithMaxLength local_best = {{}, INT_MAX};

#pragma omp for
        for (int end_vertex = 0; end_vertex < s; ++end_vertex)
        {
            std::cout << "Processing end_vertex: " << end_vertex << std::endl;
            vec_vec_int graph = graph_original;
            std::vector<subpath> subpaths(n);
            vec_bool to_use(n, true);
            bool skip = false;

            for (int i = 0; i < s; ++i)
            {
                to_use[all_stop_vertices[i]] = false;
                if (i != end_vertex)
                {
                    subpaths[all_stop_vertices[i]].path.push_back(all_stop_vertices[i]);
                    subpaths[all_stop_vertices[i]].weight = 0;
                }
            }

            vec_int stop_vertices = all_stop_vertices;
            stop_vertices.erase(stop_vertices.begin() + end_vertex);

            // ETAP I
            std::cout << "Starting ETAP I for end_vertex: " << end_vertex << std::endl;
            for (int j = 0; j < n - s; ++j)
            {
                int min_w = INT_MAX;
                int v_from = -1, v_to = -1;

                for (int v : stop_vertices)
                {
                    for (int i = 0; i < n; ++i)
                    {
                        int cur_w = graph[v][i] + subpaths[v].weight;
                        if (to_use[i] && graph[v][i] && cur_w < min_w)
                        {
                            min_w = cur_w;
                            v_from = v;
                            v_to = i;
                        }
                    }
                }

                if (min_w == INT_MAX)
                {
                    skip = true;
                    break;
                }

                subpaths[v_from].path.push_back(v_to);
                subpaths[v_from].weight += graph[v_from][v_to];
                graph[v_from] = graph[v_to];
                to_use[v_to] = false;
            }
            if (skip)
            {
                std::cout << "Skipping end_vertex: " << end_vertex << " due to incomplete path construction." << std::endl;
                // ETAP II
                std::cout << "Starting ETAP II for end_vertex: " << end_vertex << std::endl;
            }
            if (skip)
                continue;

            // ETAP II
            to_use[all_stop_vertices[end_vertex]] = true;
            int max_path_len = 0;

            for (int k = 0; k < s - 2; ++k)
            {
                int min_w = INT_MAX;
                int v_from = -1, v_to = -1;

                for (int i = 0; i < s - 2; ++i)
                {
                    for (int j = 0; j < s - 1; ++j)
                    {
                        int u = stop_vertices[i], v = stop_vertices[j];
                        int cur_w = graph[u][v] + subpaths[u].weight;
                        if (i != j && !to_use[u] && !to_use[v] && graph[u][v] && cur_w < min_w)
                        {
                            min_w = cur_w;
                            v_from = u;
                            v_to = v;
                        }
                    }
                }

                if (min_w == INT_MAX)
                {
                    skip = true;
                    break;
                }

                subpaths[v_from].path.insert(
                    subpaths[v_from].path.end(),
                    subpaths[v_to].path.begin(),
                    subpaths[v_to].path.end());

                subpaths[v_from].weight += graph[v_from][v_to];
                if (subpaths[v_from].weight > max_path_len)
                {
                    max_path_len = subpaths[v_from].weight;
                }

                subpaths[v_from].weight = subpaths[v_to].weight;
                graph[v_from] = graph[v_to];
                to_use[v_to] = true;
            }

            if (skip)
                continue;

            subpath best_sub = {};
            for (int v : stop_vertices)
            {
                if (subpaths[v].path.size() > best_sub.path.size())
                {
                    best_sub = subpaths[v];
                }
            }

            int last = best_sub.path.back();
            int final_edge = graph[last][all_stop_vertices[end_vertex]];

            if (final_edge)
            {
                int total = best_sub.weight + final_edge;
                if (total > max_path_len)
                {
                    if (max_path_len < local_best.max_length)
                    {
                        std::cout << "Updating local best for end_vertex: " << end_vertex << " with max_path_len: " << max_path_len << std::endl;
                    }

                    best_sub.path.push_back(all_stop_vertices[end_vertex]);

                    if (max_path_len < local_best.max_length)
                    {
                        local_best = {best_sub.path, max_path_len};
                    }
                }
            }

#pragma omp critical
            {
                if (local_best.max_length < best_result.max_length)
                {
                    best_result = local_best;
                }
            }
        }
    }
    return best_result;
}

int main(int argc, char **argv)
{
    Utils utils = Utils();
    for (int i = 1; i < argc; ++i)
    {
        int n, s;
        std::vector<int> stop_vertices;
        std::vector<std::vector<int>> graph;
        std::string filename = argv[i];
        utils.read_data_from_json(filename, n, s, graph, stop_vertices);
        if (!utils.is_connected(n, graph))
        {
            std::cout << "No solution for dataset '" << filename << "' : graph is not connected\n";
            continue;
        }
        PathWithMaxLength path_with_max_len = solve(n, s, graph, stop_vertices);
        if (!path_with_max_len.path.size())
        {
            std::cout << "No feasible solution for dataset '" << filename << "' found\n";
            continue;
        }
        for (int i = 0; i < path_with_max_len.path.size(); ++i)
        {
            std::cout << path_with_max_len.path[i] << ' ';
        }
        std::cout << path_with_max_len.max_length << '\n';
    }
    return 0;
}