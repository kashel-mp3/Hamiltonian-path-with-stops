#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>

using json = nlohmann::json;

std::string TEST_FOLDER = "tests";

void read_parameters_from_json(std::string filename, std::string &name, 
                                int &ntc, int &min_s, int &max_s, int &step_s,
                                int &min_n, int &max_n, int &step_n, int &max_w,
                                int &min_in_deg, int &max_in_deg, int &step_in_deg,
                                int &min_out_deg, int &max_out_deg, int &step_out_deg);
void write_data_to_json(std::string filename, std::string name, int n, int s, int in, int out, int max_w);
int digits_cnt(int num);

int main() {
    std::string params1 = "datagen_params.json";
    std::string params2 = "parameters.json";
    std::string name;
    int ntc;
    int min_s, max_s, step_s;
    int min_n, max_n, step_n;
    int max_w;
    int min_in_deg, max_in_deg, step_in_deg;
    int min_out_deg, max_out_deg, step_out_deg;
    read_parameters_from_json(params1, name, ntc, min_s, max_s, step_s, min_n, max_n, step_n,
                            max_w, min_in_deg, max_in_deg, step_in_deg, min_out_deg,
                            max_out_deg, step_out_deg);
    try {
        if (!std::filesystem::exists(TEST_FOLDER + "/" + name)) {
            std::filesystem::create_directory(TEST_FOLDER + "/" + name);
            std::cout << "Folder created successfully at: " << TEST_FOLDER << std::endl;
        } else {
            std::cout << "Folder already exists at: " << TEST_FOLDER << std::endl;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
        return 1;
    }
    int n = (max_n - min_n + step_n) / step_n;
    int s = (max_s - min_s + step_s) / step_s;
    int in_d = (max_in_deg - min_in_deg + step_in_deg) / step_in_deg;
    int out_d = (max_out_deg - min_out_deg + step_out_deg) / step_out_deg;
    int padding = digits_cnt(n * s * in_d * out_d * ntc);
    std::stringstream ss;
    int i = 0;
    int in = min_n;
    while(in <= max_n) {
        for(in; in <= max_n; in += step_n) {
            for(int is = min_s; is <= max_s; is += step_s) {
                for(int iid = min_in_deg; iid <= max_in_deg; iid += step_in_deg) {
                    for(int iod = min_out_deg; iod <= max_out_deg; iod += step_out_deg) {
                        if(is <= in && iid < in && iod < in) {
                            for(int j = 0; j < ntc; ++j){
                                ss.str("");
                                ss << i;
                                std::string padded(padding - ss.str().length(), '0');
                                padded += ss.str();
                                std::string full_name = name + "_" + padded + ".json";
                                write_data_to_json(params2, full_name, in, is, iid, iod, max_w);
                                std::string filepath = TEST_FOLDER + "/" + name + "/" + full_name;
                                std::string command = "./datagen " + params2 + " " + filepath;
                                int c = std::system(command.c_str());
                                ++i;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

void read_parameters_from_json(std::string filename, std::string &name, 
                                int &ntc, int &min_s, int &max_s, int &step_s,
                                int &min_n, int &max_n, int &step_n, int &max_w,
                                int &min_in_deg, int &max_in_deg, int &step_in_deg,
                                int &min_out_deg, int &max_out_deg, int &step_out_deg) {
    std::ifstream file(filename);
    json params;
    file >> params;

    ntc = params["number of test cases for every combination"];
    min_s = params["smallest number of stop vertices"];
    max_s = params["biggest number of stop vertices"];
    step_s = params["number of stop vertices step"];
    min_n = params["smallest number of vertices"];
    max_n = params["biggest number of vertices"];
    step_n = params["number of vertices step"];
    max_w = params["maximum edge weight"];
    min_in_deg = params["smallest maximum in degree"];
    max_in_deg = params["biggest maximum in degree"];
    step_in_deg = params["maximum in degree step"];
    min_out_deg = params["smallest maximum out degree"];
    max_out_deg = params["biggest maximum out degree"];
    step_out_deg = params["maximum out degree step"];
    name = params["test data series name"];

    file.close();
}

void write_data_to_json(std::string filename, std::string name, int n,  int s, int in, int out, int max_w) {
    std::ofstream file(filename);
    json data;

    data["number of vertices"] = n;
    data["number of stop vertices"] = s;
    data["maximum edge weight"] = max_w;
    data["maximum in degree"] = in;
    data["maximum out degree"] = out;
    data["name"] = name;

    if (file.is_open()) {
        file << std::setw(4) << data <<'\n';
        //std::cout << "test data written successfully to '" << filename << "'\n";
    } else {
        std::cerr << "unable to open file '" << filename << "'." << '\n';
    }
    file.close();
}

int digits_cnt(int num) {
    int count = 0;
    if(num == 0) {
        return 1;
    }
    while(num > 0) {
        num = num / 10;
        count++;
    }
    return count;
}