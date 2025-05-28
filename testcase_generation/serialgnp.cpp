#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <iomanip>
#include <omp.h>

using namespace std;

// ./sgnp  n p s folder_name parameter_to_test min_value max_value step num_instances

int main(int argc, char *argv[]) {
    int n = 5;
    float p = 50;
    int s = 3;
    string foldername = "../tests";
    string parameter_to_test = "n";
    int min_value = 1;
    int max_value = 10;
    int step = 1;
    int num_instances = 1;

    if (argc > 1) n = stoi(argv[1]);
    if (argc > 2) p = stof(argv[2]); // prawdopodobieństwo krawiędzi (0-100)
    if (argc > 3) s = stof(argv[3]); // procent wierzchołków postojowych (0-100), przy czym minimum będzie 2 wierzchołki
    if (argc > 4) foldername = argv[4];
    if (argc > 5) parameter_to_test = argv[5];
    if (argc > 6) min_value = stoi(argv[6]); // dla p musi być podane jak procent
    if (argc > 7) max_value = stoi(argv[7]);
    if (argc > 8) step = stoi(argv[8]);
    if (argc > 9) num_instances = stoi(argv[9]);

    p = p / 100.0f;
    cout << p << '\n'; 

    #pragma omp parallel for
    for (int value = min_value; value <= max_value; value += step) {
        int current_n = n;
        float current_p = p;
        float current_s;

        if (parameter_to_test == "n") {
            current_n = value;
        } else if (parameter_to_test == "p") {
            current_p = static_cast<float>(value) / 100.0f; 
        } else if (parameter_to_test == "s") {
            current_s = value;
        } else {
            cerr << "Invalid parameter to test: " << parameter_to_test << endl;
            exit(1);
        }

        current_s = current_n * (s / 100.0f);
        current_s = current_s < 2 ? 2 : int(current_s);

        for (int instance = 0; instance < num_instances; ++instance) {
            printf("n %d s %f p %f\n", current_n, current_s, current_p);

            string filename = foldername + "/" + to_string(instance) + "_n_" + to_string(current_n) +
                              "_p_" + to_string(static_cast<int>(current_p * 100)) +
                              "_s_" + to_string((int)current_s) + ".json";

            string command = "./gnp " + to_string(current_n) + " " +
                             to_string(current_p) + " " +
                             to_string(max_value) + " " + 
                             to_string(min_value) + " " +   
                             to_string(current_s) + " " +
                             filename;

            cout << "Running: " << command << endl;
            int ret_code = system(command.c_str());
            if (ret_code != 0) {
                cerr << "Error: Command failed with return code " << ret_code << endl;
                exit(ret_code);
            }
        }
    }

    return 0;
}