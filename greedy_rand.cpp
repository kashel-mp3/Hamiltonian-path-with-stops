#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <random>
#include <climits>

using json = nlohmann::json;
using vec_int = std::vector<int>;
using vec_bool = std::vector<bool>;
using vec_vec_int = std::vector<std::vector<int>>;

std::string FILENAME = "test.json";

struct subpath {
    vec_int path;
    int weight;
};

int solve(int n, int s, vec_vec_int &graph, vec_int stop_vertices);
bool is_connected(int n, std::vector<std::vector<int>> &graph);
void dfs(int n, int v, std::vector<std::vector<int>>& graph, std::vector<bool>& visited);
void read_data_from_json(std::string &filename, int &n, int &s, std::vector<std::vector<int>> &graph, std::vector<int> &stop_vertices);

int main() {
    int n, s;
    std::vector<int> stop_vertices;
    std::vector<std::vector<int>> graph;
    read_data_from_json(FILENAME, n, s, graph, stop_vertices);
    std::cout << solve(n, s, graph, stop_vertices) << '\n';
    return(0);
}

int solve(int n, int s, vec_vec_int &graph, vec_int all_stop_vertices) {
    if(!is_connected(n, graph)) {
        return -1;
    }
    auto rd = std::random_device {};
    auto rng = std::default_random_engine { rd() };
    std::shuffle(begin(all_stop_vertices), end(all_stop_vertices), rng);
    for(int i_left = 0; i_left < s; ++i_left) {
        std::vector<subpath> subpaths(n);
        vec_bool to_use(n, 1);
        bool skip = false;
        for(int i = 0; i < s; ++i) {
            to_use[all_stop_vertices[i]] = 0;
            if(i != i_left) {
                subpaths[all_stop_vertices[i]].path.push_back(all_stop_vertices[i]);
                subpaths[all_stop_vertices[i]].weight = 0;
            }
        }
        vec_int stop_vertices = all_stop_vertices;
        stop_vertices.erase(stop_vertices.begin() + i_left);
        // ETAP I
        for(int k = 0; k < n - s; ++k) {
            int min_w = INT_MAX;
            int v_from =  -1, v_to = -1;
            for(int v : stop_vertices) {
                for(int i = 0; i < n; ++i) {
                    if(to_use[i] && graph[v][i] && graph[v][i] + subpaths[v].weight < min_w) {
                        min_w = graph[v][i] + subpaths[v].weight;
                        v_from = v;
                        v_to = i;
                    }
                }
            }
            if(min_w == INT_MAX) {
                skip = 1;
                break;
            }
            subpaths[v_from].path.push_back(v_to);
            subpaths[v_from].weight += graph[v_from][v_to];
            graph[v_from] = graph[v_to];
            to_use[v_to] = 0;
        }
        if(skip) {
            continue;
        }
        // ETAP II
        to_use[all_stop_vertices[i_left]] = 1;
        int max_path_len = 0;
        for(int k = 0; k < s - 2; ++k) {
            int min_w = INT_MAX;
            int v_from =  -1, v_to = -1;
            for(int i = 0; i < s - 2; ++i) {
                for(int j = 0; j < s - 1; ++j) {
                    if(i != j && !to_use[stop_vertices[i]] && !to_use[stop_vertices[j]] && graph[stop_vertices[i]][stop_vertices[j]]) {
                        int cur_w = graph[stop_vertices[i]][stop_vertices[j]] + subpaths[stop_vertices[i]].weight;
                        if(cur_w < min_w ||(cur_w == min_w && rand() % 2 == 0)) {
                            min_w = graph[stop_vertices[i]][stop_vertices[j]] + subpaths[stop_vertices[i]].weight;
                            v_from = stop_vertices[i];
                            v_to = stop_vertices[j];
                        }
                    }
                }
            }
            if(min_w == INT_MAX) {
                skip = 1;
                break;
            }
            subpaths[v_from].path.insert(subpaths[v_from].path.end(), subpaths[v_to].path.begin(), subpaths[v_to].path.end());
            subpaths[v_from].weight += graph[v_from][v_to];
            if(subpaths[v_from].weight > max_path_len) {
                max_path_len = subpaths[v_from].weight;
            }
            subpaths[v_from].weight = subpaths[v_to].weight;
            graph[v_from] = graph[v_to];
            to_use[v_to] = 1;
        }
        if(skip) {
            continue;
        }
        subpath path;
        for(int v : stop_vertices) {
            if(subpaths[v].path.size() > path.path.size()) {
                path = subpaths[v];
            }
        }
        if(graph[path.path[path.path.size() - 1]][all_stop_vertices[i_left]]) {
            if(path.weight + graph[path.path[path.path.size() - 1]][all_stop_vertices[i_left]] > max_path_len) {
                max_path_len = path.weight + graph[path.path[path.path.size() - 1]][all_stop_vertices[i_left]];
            }
        } else continue;
        path.path.push_back(all_stop_vertices[i_left]);
        return max_path_len;
    }
    return -1;
}

bool is_connected(int n, std::vector<std::vector<int>> &graph) {
    std::vector<bool> visited(n, 0);
    dfs(n, 0, graph, visited);
    for (bool visit : visited) {
        if (!visit) {
            return 0;
        }
    }
    return 1;
}

void dfs(int n, int v, std::vector<std::vector<int>>& graph, std::vector<bool>& visited) {
    visited[v] = true;
    for (int i = 0; i < n; ++i) {
        if (graph[v][i] != 0 && !visited[i]) {
            dfs(n, i, graph, visited);
        }
    }
}

void read_data_from_json(std::string &filename, int &n, int &s, std::vector<std::vector<int>> &graph, std::vector<int> &stop_vertices) {
    std::ifstream file(filename);
    if (file.is_open()) {
        json data;
        file >> data;
        n = data["number of vertices"];
        s = data["number of stop vertices"];
        graph = data["graph"].get<std::vector<std::vector<int>>>();
        stop_vertices = data["stop vertices"].get<std::vector<int>>();
        file.close();
    } else {
        std::cerr << "Unable to open file '" << filename << "'." << '\n';
    }
}