import os
import json
import subprocess
import time
from collections import defaultdict
import matplotlib.pyplot as plt
import uuid

timeout = 0.5

def run_tests(program_path, test_case):
    try:
        start_time = time.time()
        result = subprocess.run([program_path, test_case], encoding='utf-8', capture_output=True, check=True, timeout=timeout)
        end_time = time.time()
        lines = result.stdout.strip().split()
        solution_value = int(lines[-1])
        return solution_value, end_time - start_time
    except subprocess.TimeoutExpired as e:
        return 0, timeout
    except Exception as e:
        print(f"some error with {program_path} {test_case}: {e}")
        exit(1)

def run_program(program_paths, test_folder):
    outputs = list()
    times = list()
    test_desc = list()
    test_cases = os.listdir(test_folder)
    test_cases.sort()
    for program_path in program_paths:
        outputs.append([])
        times.append([])
        for test_case_file in test_cases:
            with open((test_folder + '/' + test_case_file), 'r') as file:
                data = json.load(file)
                test_desc.append(data)
            test_path = test_folder + '/' + test_case_file
            output, execution_time = run_tests(program_path, test_path) 
            outputs[-1].append(output)
            times[-1].append(execution_time)
    return outputs, times, test_desc

test_folder = "./tests/a_few"
program_paths = ["./exact", "./greedy", "./genetic"]
number_of_tests = len(os.listdir(test_folder))

outputs, times, descriptions = run_program(program_paths, test_folder)

vertices_cnt = defaultdict(int)
solvable_cnt = defaultdict(int)
time_exact_vertices = defaultdict(int)
time_exact_s_to_n = defaultdict(int)
time_exact_density = defaultdict(int)
time_greedy_vertices = defaultdict(int)
time_genetic_vertices = defaultdict(int)
acc_greedy_vertices = defaultdict(int)
acc_genetic_vertices = defaultdict(int)

for i in range(number_of_tests - 1):
    vertices_cnt[int(descriptions[i]["number of vertices"])] += 1

    time_exact_vertices[int(descriptions[i]["number of vertices"])] += times[0][i]
    time_exact_s_to_n[float(descriptions[i]["number of stop vertices"]) / float(descriptions[i]["number of vertices"])] += times[0][i]
    #time_exact_density[max(float(descriptions[i]["maximum out degree"]),float(descriptions[i]["maximum in degree"])) / (float(int(descriptions[i]["number of vertices"]) - 1))] += times[0][i]

    time_greedy_vertices[int(descriptions[i]["number of vertices"])] += times[1][i]
    time_genetic_vertices[int(descriptions[i]["number of vertices"])] += times[2][i]

    if outputs[0][i]:
        solvable_cnt[int(descriptions[i]["number of vertices"])] += 1
        if(outputs[1][i] == 0 or (outputs[1][i] == -1 and outputs[0][i] != -1)):
            acc_greedy = 0
        else:
            acc_greedy = float(1) - ((float(outputs[0][i]) - float(outputs[1][i])) / float(outputs[0][i]))
        if(outputs[2][i] == 0 or (outputs[2][i] == -1 and outputs[0][i] != -1)):
            acc_genetic = 0
        else:
            acc_genetic = float(1) - ((float(outputs[0][i]) - float(outputs[2][i])) / float(outputs[0][i]))
        acc_greedy_vertices[int(descriptions[i]["number of vertices"])] += acc_greedy 
        acc_genetic_vertices[int(descriptions[i]["number of vertices"])] += acc_genetic
        
for k, n, t in zip(vertices_cnt.keys(), vertices_cnt.values(), time_exact_vertices.values()):
    time_exact_vertices[k] = float(t / n)

for k, n, t in zip(vertices_cnt.keys(), vertices_cnt.values(), time_greedy_vertices.values()):
    time_greedy_vertices[k] = float(t / n)

for k, n, t in zip(vertices_cnt.keys(), vertices_cnt.values(), time_genetic_vertices.values()):
    time_genetic_vertices[k] = float(t / n)

for k, n, a in zip(solvable_cnt.keys(), solvable_cnt.values(), acc_greedy_vertices.values()):
    acc_greedy_vertices[k] = float(a / n)

for k, n, a in zip(solvable_cnt.keys(), solvable_cnt.values(), acc_genetic_vertices.values()):
    acc_genetic_vertices[k] = float(a / n)

time_exact_vertices = {key: time_exact_vertices[key] for key in sorted(time_exact_vertices)}
time_greedy_vertices = {key: time_greedy_vertices[key] for key in sorted(time_greedy_vertices)}
time_genetic_vertices = {key: time_genetic_vertices[key] for key in sorted(time_genetic_vertices)}
acc_greedy_vertices = {key: acc_greedy_vertices[key] for key in sorted(acc_greedy_vertices)}
acc_genetic_vertices = {key: acc_genetic_vertices[key] for key in sorted(acc_genetic_vertices)}

time_exact_vertices_x = list(time_exact_vertices.keys())
time_exact_vertices_y = list(time_exact_vertices.values())

time_greedy_vertices_x = list(time_greedy_vertices.keys())
time_greedy_vertices_y = list(time_greedy_vertices.values())

time_genetic_vertices_x = list(time_genetic_vertices.keys())
time_genetic_vertices_y = list(time_genetic_vertices.values())

acc_greedy_vertices_x = list(acc_greedy_vertices.keys())
acc_greedy_vertices_y = list(acc_greedy_vertices.values())

acc_genetic_vertices_x = list(acc_genetic_vertices.keys())
acc_genetic_vertices_y = list(acc_genetic_vertices.values())

# Create unique folder for plots
unique_folder = f"plots/{uuid.uuid4()}"
os.makedirs(unique_folder, exist_ok=True)

# Plot and save time comparison
plt.figure()
plt.plot(time_exact_vertices_x, time_exact_vertices_y, label='exact')
plt.plot(time_greedy_vertices_x, time_greedy_vertices_y, label='greedy')
plt.plot(time_genetic_vertices_x, time_genetic_vertices_y, label='genetic')
plt.title('t(|V|)')
plt.xlabel('|V|')
plt.ylabel('t[s]')
plt.legend()
plt.savefig(f"{unique_folder}/time_comparison.png")

# Plot and save accuracy comparison
plt.figure()
plt.plot(acc_greedy_vertices_x, acc_greedy_vertices_y, label='greedy')
plt.plot(acc_genetic_vertices_x, acc_genetic_vertices_y, label='genetic')
plt.title('Solution Quality')
plt.xlabel('|V|')
plt.ylabel('Accuracy')
plt.legend()
plt.savefig(f"{unique_folder}/accuracy_comparison.png")

print(f"Plots saved in folder: {unique_folder}")
