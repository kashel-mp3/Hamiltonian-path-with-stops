import os
import sys
import subprocess
import time
import matplotlib.pyplot as plt

def run_program(program_path, test_file):
    try:
        start_time = time.time()
        proc = subprocess.Popen([program_path, test_file], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        while proc.poll() is None:
            if time.time() - start_time > 20:  # Timeout after 20 seconds
                proc.kill()
                print(f"Timeout: Program {program_path} took too long for {test_file}.")
                return None
        elapsed_time = time.time() - start_time
        return elapsed_time
    except subprocess.CalledProcessError as e:
        print(f"Error: Program {program_path} failed with exit code {e.returncode}")
        return None

def main():
    if len(sys.argv) != 2:
        print("Usage: python runall.py <test.json|folder>")
        sys.exit(1)

    input_path = sys.argv[1]

    if os.path.isfile(input_path):
        test_files = [input_path]
    elif os.path.isdir(input_path):
        test_files = [os.path.join(input_path, f) for f in os.listdir(input_path) if f.endswith('.json')]
        if not test_files:
            print(f"Error: No JSON files found in directory '{input_path}'.")
            sys.exit(1)
    else:
        print(f"Error: Path '{input_path}' does not exist.")
        sys.exit(1)

    base_dir = os.path.join(os.getcwd(), "exec")
    subdirs = ["parallel", "sequential"]

    all_results = {base_name: {"seq": [], "par": []} for base_name in ["greedy", "exact", "genetic"]}

    for test_file in test_files:
        print(f"Processing test file: {test_file}")
        results = {}

        for subdir in subdirs:
            program_dir = os.path.join(base_dir, subdir)

            if not os.path.isdir(program_dir):
                print(f"Error: Directory '{program_dir}' does not exist.")
                continue

            for program in reversed(os.listdir(program_dir)):
                program_path = os.path.join(program_dir, program)
                if os.path.isfile(program_path) and os.access(program_path, os.X_OK):
                    print(f"Running {program} from {subdir} with {test_file}...")
                    elapsed_time = run_program(program_path, test_file)
                    if elapsed_time is not None:
                        print(f"{program} execution time: {elapsed_time:.4f} seconds")
                        results[program] = elapsed_time

        # Calculate speedups for this test file
        for base_name in ["greedy", "exact", "genetic"]:
            seq_program = f"{base_name}_seq"
            par_program = f"{base_name}_par"
            if seq_program in results and par_program in results:
                seq_time = results[seq_program]
                par_time = results[par_program]
                speedup = seq_time / par_time if par_time > 0 else 0
                print(f"Speedup of {par_program} over {seq_program} for {test_file}: {speedup:.2f}")
                all_results[base_name]["seq"].append(seq_time)
                all_results[base_name]["par"].append(par_time)

    # Plot speedups
    plt.figure(figsize=(10, 6))
    for base_name, times in all_results.items():
        seq_times = times["seq"]
        par_times = times["par"]
        speedups = [seq / par if par > 0 else 0 for seq, par in zip(seq_times, par_times)]
        plt.plot(range(len(speedups)), speedups, label=f"{base_name} Speedup")

    plt.xlabel("Test Case Index")
    plt.ylabel("Speedup")
    plt.title("Speedup of Parallel Algorithms Over Sequential")
    plt.legend()
    plt.grid()

    name = input_path.split('/')[-1]
    plt.savefig(f"./plots/{name}.png")
    plt.show()

if __name__ == "__main__":
    main()