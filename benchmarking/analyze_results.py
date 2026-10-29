import pandas as pd
import numpy as np
import os

# Configuration
csv_file = "first_run/benchmark_results_unified.csv"
timeout_threshold = 60.0

if not os.path.exists(csv_file):
    print(f"Error: Could not find {csv_file}")
    exit(1)

# Load data
df = pd.read_csv(csv_file)

# Clean program names (e.g., 'build/bin/Release/exact_seq.exe' -> 'exact_seq')
df['Program'] = df['Program'].apply(lambda x: x.split('/')[-1].replace('.exe', '').replace('\\', '/').split('/')[-1])

# Filter out timeouts and invalid runs
valid_runs = df[(df['Solution_Value'] > 0) & (df['Execution_Time_s'] < timeout_threshold)].copy()

# ---------------------------------------------------------
# 1. Median Execution Time Table
# ---------------------------------------------------------
time_median = valid_runs.pivot_table(
    index='Vertices_N', 
    columns='Program', 
    values='Execution_Time_s', 
    aggfunc=np.median
)

# ---------------------------------------------------------
# 2. Speedup Table (Relative to exact_seq)
# ---------------------------------------------------------
if 'exact_seq' in time_median.columns:
    speedup = time_median.apply(lambda col: time_median['exact_seq'] / col)
else:
    speedup = pd.DataFrame()

# ---------------------------------------------------------
# 3. Accuracy Table (Heuristics vs Exact)
# ---------------------------------------------------------
# Extract the exact solutions to act as the baseline
exact_sols = valid_runs[valid_runs['Program'] == 'exact_seq'][['Test_Case', 'Solution_Value']]
exact_sols = exact_sols.rename(columns={'Solution_Value': 'Exact_Value'})

# Merge exact solutions with all other runs
acc_df = valid_runs.merge(exact_sols, on='Test_Case', how='inner')

# Calculate accuracy. Since this is a minimization problem, 
# Quality = (Optimal / Heuristic) * 100. (e.g., 10 / 12 = 83.3%)
acc_df['Accuracy_%'] = (acc_df['Exact_Value'] / acc_df['Solution_Value']) * 100.0

acc_median = acc_df.pivot_table(
    index='Vertices_N', 
    columns='Program', 
    values='Accuracy_%', 
    aggfunc=np.median
)

# Optional: Drop the exact_seq column from accuracy since it's always 100%
if 'exact_seq' in acc_median.columns:
    acc_median = acc_median.drop(columns=['exact_seq'])

# ---------------------------------------------------------
# Output Generation
# ---------------------------------------------------------
print("\n=== MEDIAN EXECUTION TIME (Seconds) ===")
print(time_median.to_markdown(floatfmt=".6g"))

print("\n\n=== PARALLEL SPEEDUP (Relative to exact_seq) ===")
print(speedup.round(2).to_markdown())

print("\n\n=== HEURISTIC ACCURACY (%) ===")
print(acc_median.round(2).to_markdown())

# Save to distinct CSV files for Excel/LaTeX import
output_dir = "plots"
os.makedirs(output_dir, exist_ok=True)

time_median.to_csv(f"{output_dir}/analysis_time.csv")
speedup.to_csv(f"{output_dir}/analysis_speedup.csv")
acc_median.to_csv(f"{output_dir}/analysis_accuracy.csv")

print(f"\n[!] Summaries successfully exported as CSVs to the '{output_dir}/' directory.")