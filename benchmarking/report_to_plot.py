import pandas as pd
import matplotlib.pyplot as plt
import argparse

# Parse command-line arguments
parser = argparse.ArgumentParser(description="Plot speedups as a function of parameter.")
parser.add_argument("file_path", type=str, help="Path to the input text file containing the data.")
args = parser.parse_args()

# Read data from the file
df = pd.read_csv(args.file_path, delim_whitespace=True)
print("Columns in the DataFrame:", df.columns)
print(df.head())

# Plot
plt.plot(df["parameter_value"].values, df["greedy_speedup"].values, label="Greedy Speedup", marker='o')
plt.plot(df["parameter_value"].values, df["exact_speedup"].values, label="Exact Speedup", marker='o')
plt.plot(df["parameter_value"].values, df["genetic_speedup"].values, label="Genetic Speedup", marker='o')
plt.plot(df["parameter_value"].values, df["brute_speedup"].values, label="Brute Speedup", marker='o')
# Ensure column names match the DataFrame's actual columns

# Labels and title
plt.xlabel("Parameter Value")
plt.ylabel("Speedup")
plt.title("Speedups as a Function of Parameter")
plt.xticks(rotation=45)
plt.legend()
plt.grid()

# Show plot
plt.tight_layout()
plt.savefig('plots')
plt.show()
