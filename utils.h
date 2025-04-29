#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

typedef std::vector<int> vec_int;
typedef std::vector<bool> vec_bool;
typedef std::vector<std::vector<int>> vec_vec_int;


class Utils {
public:
    static
    void read_data_from_json(std::string &filename, int &n, int &s, std::vector<std::vector<int>> &graph, std::vector<int> &stop_vertices) {
        std::ifstream file(filename);
        if (file.peek() == std::ifstream::traits_type::eof()) {
            std::cerr << "File '" << filename << "' is empty.\n";
            return;
        }
        
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
    
    static
    void write_solution_to_json(std::string &solution_path, vec_int solution, int value) {
        std::ofstream file(solution_path);
        if (file.is_open()) {
            json data;
            data["solution"] = solution;
            data["max subpath value"] = value;
            file << data;
            file.close();
        } else {
            std::cerr << "Unable to open file '" << solution_path << "'." << '\n';
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

    static
    void read_data_from_json_to_arrays(std::string &filename, int &n, int &s, int **&graph, bool *&stop_vertices) {
        std::ifstream file(filename);
        if (file.peek() == std::ifstream::traits_type::eof()) {
            std::cerr << "File '" << filename << "' is empty.\n";
            return;
        }

        if (file.is_open()) {
            json data;
            file >> data;
            n = data["number of vertices"];
            s = data["number of stop vertices"];

            graph = new int*[n];
            for (int i = 0; i < n; ++i) {
                graph[i] = new int[n];
            }

            // Fill graph
            auto graph_data = data["graph"].get<std::vector<std::vector<int>>>();
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    graph[i][j] = graph_data[i][j];
                }
            }

            stop_vertices = new bool[n];

            auto stop_vertices_data = data["stop vertices"].get<std::vector<int>>();
            for (int i = 0; i < s; ++i) {
                stop_vertices[stop_vertices_data[i]] = 1;
            }

            file.close();
        } else {
            std::cerr << "Unable to open file '" << filename << "'." << '\n';
        }
    }

    static
    void release_allocated_memory(int n, int **&graph, bool *&stop_vertices) {
        if (graph != nullptr) {
            for (int i = 0; i < n; ++i) {
                delete[] graph[i];
            }
            delete[] graph;
            graph = nullptr;
        }
        
        if (stop_vertices != nullptr) {
            delete[] stop_vertices;
            stop_vertices = nullptr;
        }
    }

    static
    bool is_connected_arrays(int n, int **graph) {
        bool *visited = new bool[n]();
        dfs_arrays(n, 0, graph, visited);
        for (int i = 0; i < n; ++i) {
            if (!visited[i]) {
                delete[] visited;
                return false;
            }
        }
        delete[] visited;
        return true;
    }

    static
    void dfs_arrays(int n, int v, int **graph, bool *visited) {
        visited[v] = true;
        for (int i = 0; i < n; ++i) {
            if (graph[v][i] != 0 && !visited[i]) {
                dfs_arrays(n, i, graph, visited);
            }
        }
    }

};