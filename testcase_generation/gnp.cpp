#include <iostream>
#include <cstdlib>
#include <string>
#include <random>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <filesystem>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

int ** AllocateA(int n) {
    int ** A = new int*[n];
    for (int i=0; i<n; i++) A[i] = new int[n];
    return A;
}

void FreeA(int n, int *** A) {
    for (int i=0; i<n; i++) delete[] (*A)[i];
    delete [] (*A);
    *A = NULL;
}

void gnp(int n, float p, int min_v, int max_v, int **A, std::mt19937 &rng) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::uniform_int_distribution<> dist_range(min_v, max_v);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < i; j++) {
            if (dist(rng) <= p) {
                A[i][j] = A[j][i] = dist_range(rng);
            } else {
                A[i][j] = A[j][i] = 0;
            }
        }
        A[i][i] = 0;
    }
}

vector<int> generate_random_subset(int n, int s, std::mt19937 &rng) {
    vector<int> sub(n);
    for (int i = 0; i < n; ++i) sub[i] = i;
    shuffle(sub.begin(), sub.end(), rng);
    sub.erase(sub.begin() + s, sub.end());
    return sub;
}

void write_data_to_json(const string &filename, int n, float p, int s, vector<vector<int>> graph, vector<int> stop_vertices) {
    std::filesystem::path file_path(filename);
    std::filesystem::create_directories(file_path.parent_path());

    ofstream file(filename);
    json data;

    data["number of vertices"] = n;
    data["edge probability"] = p;
    data["number of stop vertices"] = s;
    data["graph"] = graph;
    data["stop vertices"] = stop_vertices;

    if (file.is_open()) {
        file << setw(4) << data << '\n';
        cout << "test data written successfully to '" << filename << "'\n";
    } else {
        cerr << "unable to open file '" << filename << "'." << '\n';
    }
    file.close();
}

int main(int argc, char *argv[]) {
    int n = 5;
    float p = 0.5;
    int max_v = 100;
    int min_v = 10;
    int s = 3;
    string filename = "graph.json";

    if (argc > 1) n = stoi(argv[1]);
    if (argc > 2) p = stof(argv[2]);
    if (argc > 3) max_v = stoi(argv[3]);
    if (argc > 4) min_v = stoi(argv[4]);
    if (argc > 5) s = stoi(argv[5]);
    if (argc > 6) filename = argv[6];

    if (min_v > max_v) {
        cerr << "Error: min_v cannot be greater than max_v." << endl;
        return EXIT_FAILURE;
    }

    unsigned seed = std::random_device{}();
    std::mt19937 rng(seed);

    clog << "{n:" << n << ", p:" << p << ", min_v:" << min_v << ", max_v:" << max_v << ", s:" << s << "}" << endl;

    int **A = AllocateA(n);
    gnp(n, p, min_v, max_v, A, rng);

    vector<vector<int>> graph(n, vector<int>(n));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            graph[i][j] = A[i][j];

    vector<int> stop_vertices = generate_random_subset(n, s, rng);

    write_data_to_json(filename, n, p, s, graph, stop_vertices);

    FreeA(n, &A);
    return EXIT_SUCCESS;
}
