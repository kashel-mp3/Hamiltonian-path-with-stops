#include <nlohmann/json.hpp>
#include "../utils.h"
#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <climits>
#include <omp.h>

using json = nlohmann::json;

typedef std::vector<int> vec_int;
typedef std::vector<bool> vec_bool;
typedef std::vector<std::vector<int>> vec_vec_int;

struct PathWithMaxLength {
    std::vector<int> path;
    int max_length;
};

struct subpath {
    vec_int path;
    int weight;
};

PathWithMaxLength solve(int n, int s, vec_vec_int &graph, vec_int &all_stop_vertices) {
    for (int end_vertex = 0; end_vertex < s; ++end_vertex) {
        std::vector<subpath> subpaths(n);
        vec_bool to_use(n, 1);
        bool skip = false;
        for (int i = 0; i < s; ++i) {
            to_use[all_stop_vertices[i]] = 0;
            if (i != end_vertex) {
                subpaths[all_stop_vertices[i]].path.push_back(all_stop_vertices[i]);
                subpaths[all_stop_vertices[i]].weight = 0;
            }
        }
        vec_int stop_vertices = all_stop_vertices;
        stop_vertices.erase(stop_vertices.begin() + end_vertex);
        // ETAP I
        for (int j = 0; j < n - s; ++j) {
            int min_w = INT_MAX;
            int v_from = -1, v_to = -1;
            for (int v : stop_vertices) {
                for (int i = 0; i < n; ++i) {
                    int cur_w = graph[v][i] + subpaths[v].weight;
                    if (to_use[i] && graph[v][i] && cur_w < min_w) {
                        min_w = graph[v][i] + subpaths[v].weight;
                        v_from = v;
                        v_to = i;
                    }
                }
            }
            if (min_w == INT_MAX) {
                skip = 1;
                break;
            }
            subpaths[v_from].path.push_back(v_to);
            subpaths[v_from].weight += graph[v_from][v_to];
            graph[v_from] = graph[v_to];
            to_use[v_to] = 0;
        }
        if (skip) {
            continue;
        }
        // ETAP II
        to_use[all_stop_vertices[end_vertex]] = 1;
        int max_path_len = 0;
        for (int k = 0; k < s - 2; ++k) {
            int min_w = INT_MAX;
            int v_from = -1, v_to = -1;
            for (int i = 0; i < s - 2; ++i) {
                for (int j = 0; j < s - 1; ++j) {
                    int cur_w = graph[stop_vertices[i]][stop_vertices[j]] + subpaths[stop_vertices[i]].weight;
                    if (i != j && !to_use[stop_vertices[i]] && !to_use[stop_vertices[j]]
                        && graph[stop_vertices[i]][stop_vertices[j]] && cur_w < min_w) {
                        min_w = graph[stop_vertices[i]][stop_vertices[j]] + subpaths[stop_vertices[i]].weight;
                        v_from = stop_vertices[i];
                        v_to = stop_vertices[j];
                    }
                }
            }
            if (min_w == INT_MAX) {
                skip = 1;
                break;
            }
            subpaths[v_from].path.insert(subpaths[v_from].path.end(), subpaths[v_to].path.begin(), subpaths[v_to].path.end());
            subpaths[v_from].weight += graph[v_from][v_to];
            if (subpaths[v_from].weight > max_path_len) {
                max_path_len = subpaths[v_from].weight;
            }
            subpaths[v_from].weight = subpaths[v_to].weight;
            graph[v_from] = graph[v_to];
            to_use[v_to] = 1;
        }
        if (skip) {
            continue;
        }
        subpath path;
        for (int v : stop_vertices) {
            if (subpaths[v].path.size() > path.path.size()) {
                path = subpaths[v];
            }
        }
        if (graph[path.path[path.path.size() - 1]][all_stop_vertices[end_vertex]]) {
            if (path.weight + graph[path.path[path.path.size() - 1]][all_stop_vertices[end_vertex]] > max_path_len) {
                max_path_len = path.weight + graph[path.path[path.path.size() - 1]][all_stop_vertices[end_vertex]];
            }
        } else continue;
        path.path.push_back(all_stop_vertices[end_vertex]);
        return {path.path, max_path_len};
    }
    return {{}, 0};
}

int main(int argc, char** argv) {
    Utils utils = Utils();
    for(int i = 1; i < argc; ++i) {
        int n, s;
        std::vector<int> stop_vertices;
        std::vector<std::vector<int>> graph;
        std::string filename = argv[i];
        utils.read_data_from_json(filename, n, s, graph, stop_vertices);
        if(!utils.is_connected(n, graph)) {
            std::cout << "No solution for dataset '" << filename << "' : graph is not connected\n";
            continue;
        }
        PathWithMaxLength path_with_max_len = solve(n, s, graph, stop_vertices);
        if(!path_with_max_len.path.size()) {
            std::cout << "No feasible solution for dataset '" << filename << "' found\n";
            continue;
        } 
        for (int i = 0; i < path_with_max_len.path.size(); ++i) {
            std::cout << path_with_max_len.path[i] << ' ';
        }
        std::cout << path_with_max_len.max_length << '\n';
    }
    return 0;
}