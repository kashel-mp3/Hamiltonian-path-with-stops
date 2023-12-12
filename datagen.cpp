#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <random>
#include <algorithm>
#include <iomanip>
#include <fstream>

using json = nlohmann::json;

std::string PARAM = "parameters.json";

void generate_random_graph(std::vector<std::vector<int>> &G, int n, int max_in, int max_out, int max_w);
std::vector<int> genrate_random_subset(int n, int s);
void read_parameters_from_json(std::string &filename, int &n, int &in, int &out, int &s, int &max_w);
void write_data_to_json(std::string &filename, int n, int s, std::vector<std::vector<int>> graph, std::vector<int> stop_vertices);

int main(int argc, char** argv) {
    srand(time(NULL));
    int n, max_in_degree, max_out_degree, s, max_edge_w;
    std::vector<int> stop_vertices;
    std::vector<std::vector<int>> graph;

    read_parameters_from_json(PARAM, n, max_in_degree, max_out_degree, s, max_edge_w);
    for(int i = 1; i < argc; ++i){
        std::string file = argv[i];
        generate_random_graph(graph, n, max_in_degree, max_out_degree, max_edge_w);
        stop_vertices = genrate_random_subset(n, s);
        write_data_to_json(file, n, s, graph, stop_vertices);
    }
    return 0;
}

void init_E(int n, std::vector<std::pair<int,int>> &E) {
    for(int i = 0; i < n; ++i) {
        for(int j = 0; j < n; ++j) {
            if(i != j) {
                E.push_back({i,j});
            }
        }
    }
}

void generate_random_graph(std::vector<std::vector<int>> &G, int n, int max_in, int max_out, int max_w) {
    G = std::vector<std::vector<int>>(n, std::vector<int>(n, 0));
    std::vector<std::pair<int,int>> E;
    std::vector<int> c_in(n, 0), c_out(n, 0);
    int h = n * (n - 1);
    init_E(n, E);
    for(int i = h; i > 0; --i) {
        int z = rand() % i;
        std::pair<int,int> edge = E[z];
        if(c_out[edge.first] < max_out && c_in[edge.second] < max_in) {
            G[edge.first][edge.second] = rand() % max_w + 1;
            ++c_in[edge.second];
            ++c_out[edge.first];
            E[z] = E[i - 1];
        }
    }
}

std::vector<int> genrate_random_subset(int n, int s) {
    std::vector<int> sub(n);
    for(int i = 0; i < n; ++i) {
        sub[i] = i;
    }
    std::shuffle(sub.begin(), sub.end(), std::mt19937(std::random_device()()));
    sub.erase(sub.begin() + s, sub.end());
    return sub;
}

void read_parameters_from_json(std::string &filename, int &n, int &in, int &out, int &s, int &max_w) {
    std::ifstream file(filename);
    json params;
    file >> params;

    n = params["number of vertices"];
    in = params["maximum in degree"];
    out = params["maximum out degree"];
    s = params["number of stop vertices"];
    max_w = params["maximum edge weight"];

    file.close();
}

void write_data_to_json(std::string &filename, int n, int s, std::vector<std::vector<int>> graph, std::vector<int> stop_vertices) {
    std::ofstream file(filename);
    json data;

    data["number of vertices"] = n;
    data["number of stop vertices"] = s;
    data["graph"] = graph;
    data["stop vertices"] = stop_vertices;

    if (file.is_open()) {
        file << std::setw(4) << data <<'\n';
        std::cout << "test data written successfully to '" << filename << "'\n";
    } else {
        std::cerr << "unable to open file '" << filename << "'." << '\n';
    }
    file.close();
}