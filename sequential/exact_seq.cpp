#include <nlohmann/json.hpp>
#include "../utils.h"
#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <climits>
#include <filesystem>
#include <omp.h> 

typedef std::vector<int> vec_int;
typedef std::vector<bool> vec_bool;
typedef std::vector<std::vector<int>> vec_vec_int;

using json = nlohmann::json;

vec_int solve(int n, int s, vec_vec_int &graph, vec_int &stop_vertices, vec_bool &stop_vertices_check);
void check_all_possible_paths(int pos, int cur_l, int max_l, int used_s, vec_int path, vec_bool visited, 
                            int& min_max_l, vec_int& opt_path, int n, int s, vec_bool& stops, vec_vec_int& graph);
bool is_connected(int n, vec_vec_int &graph);
void dfs(int n, int v, vec_vec_int &graph, std::vector<bool> &visited);
int max_subpath(vec_int path, vec_bool stop_vertices_check, vec_vec_int graph);

int main(int argc, char** argv) {
    Utils utils = Utils();
    if(argc < 2) {
        std::cerr << "path to data file not provided\n";
        return 1;
    }
    std::string test_data_path = argv[1];
    int n, s;
    vec_int stop_vertices;
    vec_vec_int graph;
    utils.read_data_from_json(test_data_path, n, s, graph, stop_vertices);
    vec_bool stop_vertices_check(n, 0);
    for(auto &v : stop_vertices) {
        stop_vertices_check[v] = 1;
    }
    if(!is_connected(n, graph)) {
        std::cout << "-2 \n";
        return 0;
    }    
    vec_int solution = solve(n, s, graph, stop_vertices, stop_vertices_check);
    for(int i = 0; i < solution.size(); ++i) 
        std::cout << solution[i] << ' ';
    std::cout << max_subpath(solution, stop_vertices_check, graph) << '\n';
    return 0;
}

vec_int solve(int n, int s, vec_vec_int &graph, vec_int &stop_vertices, vec_bool &stop_vertices_check) {
    vec_int opt_path;
    int min_max_subpath = INT_MAX;
    for(int i = 0; i < s; ++i) {
        std::cout << "start vertex: " << stop_vertices[i] << '\n';
        vec_int path;
        vec_bool visited(n, 0);
        visited[stop_vertices[i]] = 1;
        path.push_back(stop_vertices[i]);
        check_all_possible_paths(1, 0, 0, 1, path, visited, min_max_subpath, opt_path, n, s, stop_vertices_check, graph);
    }
    if(min_max_subpath == INT_MAX) {
        return vec_int(0);
    }
    return opt_path;
}

void check_all_possible_paths(int pos, int cur_l, int max_l, int used_s, vec_int path, vec_bool visited, 
                            int& min_max_l, vec_int& opt_path, int n, int s, vec_bool& stops, vec_vec_int& graph) {
    if (pos == n) {
        if(max_l < min_max_l) {
            min_max_l = max_l;
        }
        opt_path = path;
        return;
    }
    for (int v = 0; v < n; v++) {
        if (graph[path[pos - 1]][v] && !visited[v]) {
            path.push_back(v);
            visited[v] = 1;
            int new_max_l = max_l;
            int new_cur_l = cur_l + graph[path[pos - 1]][v];
            int new_used_s = used_s;
            if(stops[v]) {
                ++new_used_s;
                if(new_used_s == s && pos != n - 1) {
                    visited[v] = 0;
                    path.pop_back();
                    continue;
                }
                if(new_max_l < new_cur_l) {
                    if(new_cur_l >= min_max_l) {
                        visited[v] = 0;
                        path.pop_back();
                        continue;
                    }
                    new_max_l = new_cur_l;
                }
                new_cur_l = 0;
            }
            check_all_possible_paths(pos + 1, new_cur_l, new_max_l, new_used_s, path, visited, min_max_l, opt_path, n, s, stops, graph);
            visited[v] = 0;
            path.pop_back();
        }
    }
}

bool is_connected(int n, vec_vec_int &graph) {
    std::vector<bool> visited(n, 0);
    dfs(n, 0, graph, visited);
    for (bool visit : visited) {
        if (!visit) {
            return 0;
        }
    }
    return 1;
}

void dfs(int n, int v, vec_vec_int& graph, std::vector<bool>& visited) {
    visited[v] = true;
    for (int i = 0; i < n; ++i) {
        if (graph[v][i] != 0 && !visited[i]) {
            dfs(n, i, graph, visited);
        }
    }
}

int max_subpath(vec_int path, vec_bool stop_vertices_check, vec_vec_int graph) {
    int max_subpath = 0;
    int cur_subpath = 0;
    if(!path.size()) {
        return -1;
    }
    for(int i = 1; i < path.size(); ++i) {
        cur_subpath += graph[path[i - 1]][path[i]];
        if(stop_vertices_check[path[i]]) {
            if(max_subpath < cur_subpath) {
                max_subpath = cur_subpath;
            }
            cur_subpath = 0;
        }
    }
    return max_subpath;
}