import os
import sys
import csv
import subprocess
import time
from collections import defaultdict
import numpy as np

def parse_test_case_name(file_name):
    base_name = os.path.basename(file_name)
    parts = base_name.split('/')[-1]
    parts = parts.split('_')
    n = parts[2] 
    p = parts[4] 
    s = parts[6].split('.')[0]  
    return int(n), int(p), int(s)

def execute_test_case(program_path, test_case_path, num_cores, timeout=60*60*2):
    start_time = time.time()
    try:
        result = subprocess.run(
            [program_path, test_case_path, str(num_cores)],
            capture_output=True,
            text=True,
            check=True,
            timeout=timeout
        )
        end_time = time.time()
        
        # Check if fast algorithm time is provided (e.g. greedy, genetic)
        lines = result.stdout.strip().split()
        if len(lines) >= 2 and ('greedy' in program_path or 'genetic' in program_path):
            execution_time = float(lines[-1]) / 1_000_000.0
            program_output = lines[-2]
        else:
            execution_time = end_time - start_time
            program_output = lines[-1] if lines else ""
            
        return execution_time, program_output
    except subprocess.TimeoutExpired:
        execution_time = time.time() - start_time
        return execution_time, "Error: Execution timed out"
    except subprocess.CalledProcessError as e:
        execution_time = time.time() - start_time
        return execution_time, f"Error: {e.stderr.strip()}"
    
def main():
    if len(sys.argv) != 5:
        print("Usage: python run.py <program_path> <test_cases_folder> <min_num_cores> <max_num_cores>")
        sys.exit(1)

    program_path = sys.argv[1]
    test_cases_folder = sys.argv[2]
    min_num_cores = int(sys.argv[3])
    max_num_cores = int(sys.argv[4])

    report_file = os.path.basename(program_path) + "_raport.csv"
    
    test_cases = [
        test_case for test_case in os.listdir(test_cases_folder) if test_case.endswith('.json')
    ]
    test_cases.sort(key=lambda tc: parse_test_case_name(tc)[0])

    # Dictionary to aggregate results: key = (num_cores, n, p, s), value = list of times
    results_agg = defaultdict(list)
    output_agg = defaultdict(list)

    for num_cores in range(min_num_cores, max_num_cores + 1):
        for test_case in test_cases:
            test_case_path = os.path.join(test_cases_folder, test_case)
            n, p, s = parse_test_case_name(test_case)
            execution_time, program_output = execute_test_case(program_path, test_case_path, num_cores)
            
            results_agg[(num_cores, n, p, s)].append(execution_time)
            output_agg[(num_cores, n, p, s)].append(program_output)
            print(num_cores, n, p, s, execution_time, program_output)

    with open(report_file, mode='w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['n_cores', 'n', 'p', 's', 'execution_time_median', 'execution_time_iqr_q1', 'execution_time_iqr_q3', 'program_output_sample'])
        
        for key in sorted(results_agg.keys()):
            times = results_agg[key]
            median = np.median(times)
            if len(times) > 1:
                q1, q3 = np.percentile(times, [25, 75])
            else:
                q1, q3 = median, median
                
            sample_out = output_agg[key][0]
            writer.writerow([key[0], key[1], key[2], key[3], median, q1, q3, sample_out])

if __name__ == "__main__":
    main()