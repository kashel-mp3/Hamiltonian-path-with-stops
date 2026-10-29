import os
import json
import subprocess
import time
import csv
from collections import defaultdict
import matplotlib.pyplot as plt
import numpy as np
import uuid

timeout = 600.0  # Seconds before exact_seq gets killed
log_file = "benchmark_progress.log"
csv_file = "benchmark_results.csv"

def log_progress(message):
    """Appends a timestamped message to the log file."""
    with open(log_file, "a", encoding="utf-8") as f:
        f.write(message + "\n")

def run_tests(program_path, test_case):
    try:
        start_time = time.time()
        result = subprocess.run([program_path, test_case], encoding='utf-8', capture_output=True, check=True, timeout=timeout)
        end_time = time.time()
        
        lines = result.stdout.strip().split()
        if len(lines) == 0:
            return 0, end_time - start_time
            
        # Catch C++ specific error outputs (-1 or -2)
        if lines[0] in ['-1', '-2', '0']:
            return 0, end_time - start_time

        # Fast heuristics output: <path> <solution_value> <time_in_microseconds>
        if 'greedy' in program_path or 'genetic' in program_path:
            if len(lines) >= 2:
                time_in_sec = float(lines[-1]) / 1_000_000.0
                solution_value = int(lines[-2])
                return solution_value, time_in_sec
            return 0, 0
            
        # CUDA output: <path> <solution_value> <kernel_ms> <host_ms>
        elif 'cuda' in program_path:
            time_in_sec = float(lines[-1]) / 1000.0 
            solution_value = int(lines[-3])
            return solution_value, time_in_sec
            
        # Exact CPU output: <path> <solution_value>
        else:
            solution_value = int(lines[-1])
            return solution_value, end_time - start_time
            
    except subprocess.TimeoutExpired:
        return 0, timeout
    except Exception as e:
        log_progress(f"ERROR executing {program_path} on {test_case}: {e}")
        return 0, 0

def run_program(program_paths, test_folder):
    outputs = []
    times = []
    test_desc = []
    
    # Safely load and properly sort instances mathematically by n
    test_cases = [tc for tc in os.listdir(test_folder) if tc.endswith('.json')]
    test_cases.sort(key=lambda x: int(x.split('_n_')[1].split('_')[0]))

    log_progress("Loading JSON descriptions...")
    for test_case_file in test_cases:
        with open(os.path.join(test_folder, test_case_file), 'r') as file:
            test_desc.append(json.load(file))

    # Initialize CSV file with headers
    with open(csv_file, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["Program", "Test_Case", "Vertices_N", "Stops_S", "Solution_Value", "Execution_Time_s"])

    for program_path in program_paths:
        outputs.append([])
        times.append([])
        consecutive_timeouts = 0  
        
        log_progress(f"\n--- Starting benchmark for {program_path} ---")
        
        for i, test_case_file in enumerate(test_cases):
            test_path = os.path.join(test_folder, test_case_file)
            n_val = test_desc[i]["number of vertices"]
            s_val = test_desc[i]["number of stop vertices"]
            
            # Safeguard: Skip larger instances if the previous 2 timed out
            if consecutive_timeouts >= 2:
                log_progress(f"SKIPPED: {test_case_file} (due to previous timeouts)")
                outputs[-1].append(0)
                times[-1].append(timeout)
                
                # Immediately log skipped run to CSV
                with open(csv_file, "a", newline="", encoding="utf-8") as f:
                    csv.writer(f).writerow([program_path, test_case_file, n_val, s_val, 0, timeout])
                continue
                
            log_progress(f"RUNNING: {test_case_file}...")
            output, execution_time = run_tests(program_path, test_path)
            
            if execution_time >= timeout:
                log_progress(f"TIMEOUT: {test_case_file} exceeded {timeout}s.")
                consecutive_timeouts += 1
            else:
                log_progress(f"SUCCESS: {test_case_file} finished in {execution_time:.4f}s.")
                consecutive_timeouts = 0
                
            outputs[-1].append(output)
            times[-1].append(execution_time)
            
            # IMMEDIATELY log successful/timeout run to CSV
            with open(csv_file, "a", newline="", encoding="utf-8") as f:
                csv.writer(f).writerow([program_path, test_case_file, n_val, s_val, output, execution_time])
            
    return outputs, times, test_desc

def compute_stats(data_dict):
    """Calculates Medians and Interquartile Ranges (IQR) for error bars"""
    x = sorted(data_dict.keys())
    y_median = []
    y_err_lower = []
    y_err_upper = []
    
    for k in x:
        data = data_dict[k]
        if len(data) == 0:
            continue
        median = np.median(data)
        if len(data) > 1:
            q1, q3 = np.percentile(data, [25, 75])
            y_err_lower.append(median - q1)
            y_err_upper.append(q3 - median)
        else:
            y_err_lower.append(0)
            y_err_upper.append(0)
        y_median.append(median)
        
    return x, y_median, [y_err_lower, y_err_upper]

if __name__ == "__main__":
    # Clear out the old log file for a fresh run
    with open(log_file, "w", encoding="utf-8") as f:
        f.write("=== BENCHMARK RUN LOG ===\n")
        
    test_folder = "./tests/a_few"
    if not os.path.exists(test_folder):
        print(f"Directory {test_folder} not found. Please generate tests first.")
        exit(0)
        
    potential_programs = [
        # --- Baseline Exact (Must be first for accuracy calculations) ---
        #"build/bin/Release/exact_seq.exe", 
        
        # --- Sequential Heuristics ---
        #"build/bin/Release/greedy_seq.exe", 
        #"build/bin/Release/genetic_seq.exe",
        
        # --- OpenMP Parallel Solvers ---
        #"build/bin/Release/exact_par.exe",
        #"build/bin/Release/greedy_par.exe",
        "build/bin/Release/genetic_par.exe",
        
        # --- CUDA GPU Solvers ---
        #"build/bin/Release/exact_cuda_v.exe",
        #"build/bin/Release/exact_cuda_e.exe"
    ]
    
    program_paths = []
    for p in potential_programs:
        if os.path.exists(p):
            program_paths.append(p)
        else:
            log_progress(f"WARNING: Executable not found at {p}")
            
    if not program_paths:
        print("No valid executables found. Check your paths!")
        exit(0)

    # Run the benchmarks
    outputs, times, descriptions = run_program(program_paths, test_folder)
    
    # Aggregate data by graph size (|V|)
    time_data = [defaultdict(list) for _ in program_paths]
    acc_data = [defaultdict(list) for _ in program_paths]
    
    log_progress("\nAggregating statistical results...")
    
    for i in range(len(descriptions)):
        # FIX: Access the dictionary properly with [i]
        v = int(descriptions[i]["number of vertices"])
        
        # Exact solution is the baseline (index 0)
        exact_ans = outputs[0][i] if len(outputs) > 0 else 0
        
        for p_idx, program in enumerate(program_paths):
            time_data[p_idx][v].append(times[p_idx][i])
            
            ans = outputs[p_idx][i]
            if exact_ans > 0 and ans > 0:
                acc = 1.0 - ((exact_ans - ans) / exact_ans)
                acc_data[p_idx][v].append(acc)
            else:
                acc_data[p_idx][v].append(0)
                
    # Create unique output directory
    unique_folder = f"plots/{uuid.uuid4()}"
    os.makedirs(unique_folder, exist_ok=True)
    
    # Plot Execution Times
    plt.figure()
    for p_idx, program in enumerate(program_paths):
        name = os.path.basename(program).replace('.exe', '')
        x, y_med, y_err = compute_stats(time_data[p_idx])
        plt.errorbar(x, y_med, yerr=y_err, label=name, fmt='-o', capsize=5)
        
    plt.title('Execution Time t(|V|) - Median with IQR')
    plt.xlabel('|V| (Number of Vertices)')
    plt.ylabel('Time [s]')
    plt.legend()
    plt.savefig(f"{unique_folder}/time_comparison.png")
    
    # Plot Solution Quality (Accuracy)
    plt.figure()
    for p_idx, program in enumerate(program_paths):
        if 'exact' in program: 
            continue # Exact is always the 100% baseline
        name = os.path.basename(program).replace('.exe', '')
        x, y_med, y_err = compute_stats(acc_data[p_idx])
        plt.errorbar(x, y_med, yerr=y_err, label=name, fmt='-o', capsize=5)
        
    plt.title('Solution Quality - Median with IQR')
    plt.xlabel('|V| (Number of Vertices)')
    plt.ylabel('Accuracy')
    plt.legend()
    plt.savefig(f"{unique_folder}/accuracy_comparison.png")
    
    completion_msg = f"Plots successfully generated and saved in: {unique_folder}"
    print(completion_msg)
    log_progress(f"\n{completion_msg}")