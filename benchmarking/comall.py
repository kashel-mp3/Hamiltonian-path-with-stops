import os
import subprocess

programs = ["parallel/greedy_par", "parallel/genetic_par", "parallel/exact_par", "parallel/brute_par",
            "sequential/greedy_seq", "sequential/exact_seq", "sequential/genetic_seq", "sequential/brute_seq"]

# Directory for compiled executables
output_dir = "exec"
os.makedirs(output_dir, exist_ok=True)

# Compile each program
for program in programs:
    source_file = f"{program}.cpp"
    output_file = os.path.join(output_dir, program)
    compile_command = ["g++", source_file, "-o", output_file]
    
    if "par" in program:
        compile_command.insert(2, "-fopenmp")
    
    try:
        subprocess.run(compile_command, check=True)
        print(f"Compiled {source_file} to {output_file}")
    except subprocess.CalledProcessError as e:
        print(f"Failed to compile {source_file}: {e}")