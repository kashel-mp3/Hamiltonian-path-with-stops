import os
import sys
import subprocess
import time
import matplotlib.pyplot as plt
import numpy as np # Needed for handling potential None values in plots

ALOT = 30 # Timeout in seconds
ALL_POSSIBLE_BASE_NAMES = ["greedy", "exact", "brute", "genetic"] # Define all known algorithms

def run_program(program_path, test_file):
    """Runs a single program with a timeout and returns execution time."""
    try:
        start_time = time.time()
        # Use Popen for non-blocking check and timeout
        proc = subprocess.Popen([program_path, test_file], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        while proc.poll() is None:
            if time.time() - start_time > ALOT:
                proc.kill()
                # Wait a bit to ensure process is killed before accessing stderr/stdout
                proc.wait(timeout=1)
                print(f"Timeout (> {ALOT}s): Program {os.path.basename(program_path)} for {os.path.basename(test_file)}.")
                # Capture any partial output if needed
                # stdout, stderr = proc.communicate()
                # print(f"Partial stdout:\n{stdout}")
                # print(f"Partial stderr:\n{stderr}")
                return None # Indicate timeout specifically maybe? Or stick with None for failure/timeout
            time.sleep(0.1) # Small sleep to prevent busy-waiting

        # Process finished within timeout
        elapsed_time = time.time() - start_time
        stdout, stderr = proc.communicate() # Get final output

        if proc.returncode != 0:
            print(f"Error: Program {os.path.basename(program_path)} failed for {os.path.basename(test_file)} (exit code {proc.returncode}).")
            print(f"Stderr:\n{stderr}")
            return None
        else:
            # Optional: Print stdout if needed for debugging successful runs
            # print(f"Stdout:\n{stdout}")
             return elapsed_time

    except FileNotFoundError:
        print(f"Error: Program not found at '{program_path}'")
        return None
    except subprocess.TimeoutExpired:
         # This might happen if proc.wait() times out after kill, unlikely but handle it
         print(f"Error: Process cleanup timed out for {os.path.basename(program_path)} on {os.path.basename(test_file)}.")
         return None
    except Exception as e:
        print(f"An unexpected error occurred while running {os.path.basename(program_path)} on {os.path.basename(test_file)}: {e}")
        return None

def sort_test_files(test_files):
    def extract_key(file_name):
        file_name = file_name.split('/')[-1]
        file_name = file_name.split('.')[0]
        parts = file_name.split('_')
        try:
            n = int(parts[1])  # Extract n value
            s = int(parts[3])  # Extract s value
            z = int(parts[5])  # Extract z value
            return n, s, z
        except (IndexError, ValueError):
            raise ValueError(f"Error: Could not parse parameters from filename '{file_name}'.")

    return sorted(test_files, key=lambda f: extract_key(f))

def main():
    # --- Argument Parsing ---
    if len(sys.argv) < 3:
        print("Usage: python runall.py <test.json|folder> <algorithm1> [algorithm2] ...")
        print(f"Available algorithms: {', '.join(ALL_POSSIBLE_BASE_NAMES)}")
        sys.exit(1)

    input_path = sys.argv[1]
    # Use a set for efficient checking of requested algorithms
    requested_algorithms = set(sys.argv[2:])

    # --- Validate Requested Algorithms ---
    valid_requested_algorithms = set()
    for algo in requested_algorithms:
        if algo in ALL_POSSIBLE_BASE_NAMES:
            valid_requested_algorithms.add(algo)
        else:
            print(f"Warning: Unknown algorithm '{algo}' specified and will be ignored.")

    if not valid_requested_algorithms:
        print("Error: No valid algorithms specified to run.")
        sys.exit(1)

    print(f"Requested algorithms to run: {', '.join(sorted(list(valid_requested_algorithms)))}")


    # --- Test File Handling ---
    if os.path.isfile(input_path):
        test_files = [input_path]
        input_name = os.path.splitext(os.path.basename(input_path))[0] # Name for report/plot
    elif os.path.isdir(input_path):
        test_files = [os.path.join(input_path, f) for f in os.listdir(input_path) if f.endswith('.json')]
        test_files = sort_test_files(test_files)
        if not test_files:
            print(f"Error: No JSON files found in directory '{input_path}'.")
            sys.exit(1)
        test_files = sort_test_files(test_files)
        input_name = os.path.basename(os.path.normpath(input_path)) # Use folder name
    else:
        print(f"Error: Path '{input_path}' is not a valid file or directory.")
        sys.exit(1)

    # --- Setup Directories ---
    base_dir = os.path.join(os.getcwd(), "exec")
    subdirs = ["sequential", "parallel"] # Define order for consistency

    # --- Results Initialization ---
    # Store results indexed by test file, then program name
    results_by_file = {os.path.basename(f): {} for f in test_files}
    # Store aggregated results for plotting/reporting (using None for missing data)
    # Initialize for all possible algorithms to maintain consistent report structure
    aggregated_results = {base_name: {"seq": [None] * len(test_files), "par": [None] * len(test_files)}
                          for base_name in ALL_POSSIBLE_BASE_NAMES}


    # --- Main Execution Loop ---
    for i, test_file in enumerate(test_files):
        test_basename = os.path.basename(test_file)
        print(f"\n--- Processing test file: {test_basename} ({i+1}/{len(test_files)}) ---")

        for subdir in subdirs:
            program_dir = os.path.join(base_dir, subdir)

            if not os.path.isdir(program_dir):
                print(f"Warning: Directory '{program_dir}' not found. Skipping.")
                continue

            # Iterate through files in the directory
            try:
                program_files = os.listdir(program_dir)
            except OSError as e:
                 print(f"Error listing files in '{program_dir}': {e}. Skipping.")
                 continue

            for program_filename in program_files:
                program_path = os.path.join(program_dir, program_filename)

                # Basic check if it's a file and executable
                if not (os.path.isfile(program_path) and os.access(program_path, os.X_OK)):
                    continue

                # --- Algorithm Filtering ---
                parts = program_filename.split('_')
                if len(parts) < 2: # Expecting name_seq or name_par
                    continue
                base_name = parts[0]
                prog_type = parts[-1] # Should be 'seq' or 'par'

                # Check if the algorithm's base name was requested
                if base_name in valid_requested_algorithms:
                    print(f"Running {program_filename}...")
                    elapsed_time = run_program(program_path, test_file)

                    # Store result if successful
                    if elapsed_time is not None:
                        print(f"  -> {program_filename} time: {elapsed_time:.4f}s")
                        # Store in results_by_file for detailed access if needed
                        results_by_file[test_basename][program_filename] = elapsed_time
                        # Store in aggregated_results for plotting/reporting
                        if prog_type == 'seq':
                            aggregated_results[base_name]["seq"][i] = elapsed_time
                        elif prog_type == 'par':
                            aggregated_results[base_name]["par"][i] = elapsed_time
                    else:
                         print(f"  -> {program_filename} failed or timed out.")
                         # Ensure None is stored in aggregated results
                         if prog_type == 'seq':
                            aggregated_results[base_name]["seq"][i] = None
                         elif prog_type == 'par':
                            aggregated_results[base_name]["par"][i] = None

                # else: # Optional: uncomment to see skipped programs
                #     print(f"Skipping {program_filename} (algorithm '{base_name}' not requested).")


    # --- Process Results for Reporting and Plotting ---
    print("\n--- Results Summary ---")
    report_data = []
    plot_labels = [os.path.splitext(os.path.basename(f))[0] for f in test_files] # Labels for plot x-axis

    for i, test_basename in enumerate(results_by_file.keys()):
        row = [plot_labels[i]] # Start row with test file identifier
        print(f"\nTest File: {test_basename}")

        # Iterate through ALL possible algorithms for consistent report structure
        for base_name in ALL_POSSIBLE_BASE_NAMES:
            seq_time = aggregated_results[base_name]["seq"][i]
            par_time = aggregated_results[base_name]["par"][i]

            # Check if this algorithm was requested AND ran successfully
            if base_name in valid_requested_algorithms:
                 if seq_time is not None and par_time is not None:
                    speedup = seq_time / par_time if par_time > 0 else float('inf') # Handle par_time = 0
                    print(f"  {base_name:<10}: Seq={seq_time:.4f}s, Par={par_time:.4f}s, Speedup={speedup:.2f}x")
                    row.extend([f"{seq_time:.4f}", f"{par_time:.4f}", f"{speedup:.2f}"])
                 else:
                    # Requested but one or both failed/timed out
                    print(f"  {base_name:<10}: Data missing (Seq: {seq_time}, Par: {par_time})")
                    row.extend(["N/A", "N/A", "N/A"])
            else:
                 # Algorithm was not requested, fill with N/A for report consistency
                 row.extend(["N/A", "N/A", "N/A"])

        report_data.append(row)


    # --- Plot Speedups ---
    if any(aggregated_results[bn]["seq"][i] is not None and aggregated_results[bn]["par"][i] is not None
           for bn in valid_requested_algorithms for i in range(len(test_files))): # Check if there's anything to plot

        plt.figure(figsize=(12, 7)) # Adjusted size
        for base_name in ALL_POSSIBLE_BASE_NAMES:
             # Only plot algorithms that were requested
            if base_name in valid_requested_algorithms:
                seq_times = aggregated_results[base_name]["seq"]
                par_times = aggregated_results[base_name]["par"]

                # Prepare data for plotting: filter out None values
                plot_x_indices = []
                plot_speedups = []
                for i, (s, p) in enumerate(zip(seq_times, par_times)):
                    if s is not None and p is not None:
                         plot_x_indices.append(i)
                         plot_speedups.append(s / p if p > 0 else np.inf) # Calculate speedup

                # Use indices for plotting, then set labels
                if plot_x_indices: # Only plot if there's valid data
                    plt.plot(plot_x_indices, plot_speedups, label=f"{base_name} Speedup", marker='o', linestyle='-')

        plt.xlabel("Test Case")
        plt.ylabel("Speedup (Sequential Time / Parallel Time)")
        plt.title(f"Algorithm Speedups for '{input_name}'")
        # Set x-axis ticks and labels based on the actual test files run
        plt.xticks(ticks=range(len(plot_labels)), labels=plot_labels, rotation=45, ha='right')
        plt.legend()
        plt.grid(True, which='both', linestyle='--', linewidth=0.5)
        plt.tight_layout() # Adjust layout to prevent labels overlapping

        # Save plot
        plot_dir = "./plots"
        os.makedirs(plot_dir, exist_ok=True)
        plot_filename = os.path.join(plot_dir, f"{input_name}_speedup.png")
        counter = 1
        while os.path.exists(plot_filename):
             plot_filename = os.path.join(plot_dir, f"{input_name}_speedup_{counter}.png")
             counter += 1
        plt.savefig(plot_filename)
        print(f"\nPlot saved to {plot_filename}")
        plt.show()
    else:
         print("\nNo valid speedup data generated to plot.")


    # --- Write Report ---
    # Write report only if input was a directory (multiple tests) and there's data
    if os.path.isdir(input_path) and report_data:
        report_dir = "./raports" # Consistent spelling with original script
        os.makedirs(report_dir, exist_ok=True)
        report_file = os.path.join(report_dir, f"{input_name}_report.txt")
        counter = 1
        while os.path.exists(report_file):
            report_file = os.path.join(report_dir, f"{input_name}_report_{counter}.txt")
            counter += 1

        with open(report_file, "w") as f:
            # Create header with ALL possible algorithms for consistency
            header_parts = ["test_case"]
            for bn in ALL_POSSIBLE_BASE_NAMES:
                header_parts.extend([f"{bn}_seq_t", f"{bn}_par_t", f"{bn}_speedup"])
            header = " ".join(header_parts) + "\n"
            f.write(header)

            # Write data rows (already contain 'N/A' where needed)
            for row in report_data:
                f.write(" ".join(str(x) for x in row) + "\n")
        print(f"Report saved to {report_file}")
    elif os.path.isfile(input_path):
         print("\nReport generation skipped (input was a single file).")
    else:
         print("\nReport generation skipped (no data).")


if __name__ == "__main__":
    main()