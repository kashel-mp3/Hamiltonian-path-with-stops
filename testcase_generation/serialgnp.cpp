#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <iomanip>
#include <filesystem>
#include <omp.h>

using namespace std;

#ifndef DEFAULT_GNP_EXE
#define DEFAULT_GNP_EXE "gnp"
#endif

int main(int argc, char *argv[]) {
    int n = 5;
    float p = 50;
    int s = 3;
    string foldername = "../tests";
    string parameter_to_test = "n";
    int min_value = 1;
    int max_value = 10;
    int step = 1;
    int num_instances = 30;

    string gnp_exe = filesystem::path(DEFAULT_GNP_EXE).make_preferred().string();

    if (argc > 1) n = stoi(argv[1]);
    if (argc > 2) p = stof(argv[2]); 
    if (argc > 3) s = stof(argv[3]); 
    if (argc > 4) foldername = argv[4];
    if (argc > 5) parameter_to_test = argv[5];
    if (argc > 6) min_value = stoi(argv[6]); 
    if (argc > 7) max_value = stoi(argv[7]);
    if (argc > 8) step = stoi(argv[8]);
    if (argc > 9) num_instances = stoi(argv[9]);
    if (const char *env_exe = getenv("GNP_EXE")) gnp_exe = env_exe;
    if (argc > 10) gnp_exe = argv[10];

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

        if (parameter_to_test != "s") {
            current_s = current_n * (s / 100.0f);
            current_s = current_s < 2 ? 2 : int(current_s);
        }

        for (int instance = 0; instance < num_instances; ++instance) {
            printf("n %d s %f p %f\n", current_n, current_s, current_p);

            string filename = (filesystem::path(foldername) /
                               (to_string(instance) + "_n_" + to_string(current_n) +
                                "_p_" + to_string(static_cast<int>(current_p * 100)) +
                                "_s_" + to_string((int)current_s) + ".json"))
                                  .make_preferred()
                                  .string();

            string command = "\"" + gnp_exe + "\" " + to_string(current_n) + " " +
                             to_string(current_p) + " " +
                             to_string(max_value) + " " + 
                             to_string(min_value) + " " +   
                             to_string(current_s) + " " +
                             "\"" + filename + "\"";

            cout << "Running: " << command << endl;
#ifdef _WIN32
            int ret_code = system(("\"" + command + "\"").c_str());
#else
            int ret_code = system(command.c_str());
#endif
            if (ret_code != 0) {
                cerr << "Error: Command failed with return code " << ret_code << endl;
                exit(ret_code);
            }
        }
    }

    return 0;
}