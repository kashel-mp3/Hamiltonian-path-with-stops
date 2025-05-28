import os
import sys
import csv
import subprocess
import time

def parse_test_case_name(file_name):
    """Extract n, p, s values from the test case file name."""
    base_name = os.path.basename(file_name)
    parts = base_name.split('/')[-1]
    parts = parts.split('_')
    n = parts[2] 
    p = parts[4] 
    s = parts[6].split('.')[0]  
    return n, p, s

def execute_test_case(program_path, test_case_path, num_cores, timeout=60*60*2):
    """Execute the program with the test case and return execution time and output."""
    start_time = time.time()
    try:
        result = subprocess.run(
            [program_path, test_case_path, str(num_cores)],
            capture_output=True,
            text=True,
            check=True,
            timeout=timeout
        )
        execution_time = time.time() - start_time
        return execution_time, result.stdout.strip()
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
    file_exists = os.path.isfile(report_file)
    with open(report_file, mode='a', newline='') as csvfile:
        writer = csv.writer(csvfile)
        if not file_exists:
            writer.writerow(['n_cores', 'n', 'p', 's', 'execution time', 'program output'])

        test_cases = [
            test_case for test_case in os.listdir(test_cases_folder) if test_case.endswith('.json')
        ]
        test_cases.sort(key=lambda tc: int(parse_test_case_name(tc)[0]))  # Sort by 'n'

        for num_cores in range(min_num_cores, max_num_cores + 1):
            for test_case in test_cases:
                test_case_path = os.path.join(test_cases_folder, test_case)
                n, p, s = parse_test_case_name(test_case)
                execution_time, program_output = execute_test_case(program_path, test_case_path, num_cores)
                print(num_cores, n, p, s, execution_time, program_output)
                writer.writerow([num_cores, n, p, s, execution_time, program_output])

if __name__ == "__main__":
    main()