import pandas as pd
import numpy as np
import os

# --- CONFIGURATION ---
file_initial = "first_run/benchmark_results.csv"
file_overnight = "first_run/benchmark_results_overnight.csv"
unified_csv = "first_run/benchmark_results_unified.csv"
TIMEOUT_STR = ">600.0"
TIMEOUT_NUM = 600.0

# 1. Load and Merge Data
dfs = []
if os.path.exists(file_initial):
    dfs.append(pd.read_csv(file_initial))
if os.path.exists(file_overnight):
    dfs.append(pd.read_csv(file_overnight))

if not dfs:
    print("Error: Could not find the CSV files.")
    exit(1)

df = pd.concat(dfs, ignore_index=True)

# Clean program names
df['Program'] = df['Program'].apply(lambda x: x.split('/')[-1].replace('.exe', '').replace('\\', '/').split('/')[-1])

# 2. Unify Duplicates (Prioritize valid solutions, then longest execution time)
df['is_valid'] = df['Solution_Value'] > 0
df.sort_values(by=['is_valid', 'Execution_Time_s'], ascending=[False, False], inplace=True)
df_merged = df.drop_duplicates(subset=['Program', 'Test_Case'], keep='first').copy()
df_merged.drop(columns=['is_valid'], inplace=True)

# Save the unified CSV
df_merged.to_csv(unified_csv, index=False)
print(f"[+] Saved unified data to {unified_csv}\n")

all_n = sorted(df_merged['Vertices_N'].unique())
all_progs = sorted(df_merged['Program'].unique())

# 3. Median Execution Time Table
time_table = pd.DataFrame(index=all_n, columns=all_progs)

for n in all_n:
    for p in all_progs:
        subset = df_merged[(df_merged['Vertices_N'] == n) & (df_merged['Program'] == p)]
        if subset.empty:
            continue
            
        valid_subset = subset[subset['Solution_Value'] > 0]
        if valid_subset.empty:
            time_table.loc[n, p] = TIMEOUT_STR
        else:
            time_table.loc[n, p] = np.median(valid_subset['Execution_Time_s'])

time_print = time_table.copy()
for col in time_print.columns:
    time_print[col] = time_print[col].apply(lambda x: f"{x:.6g}" if isinstance(x, float) else str(x))

# 4. Parallel Speedup Table (Relative to exact_seq)
speedup_table = pd.DataFrame(index=all_n, columns=all_progs)

for n in all_n:
    val_seq = time_table.loc[n, 'exact_seq'] if 'exact_seq' in time_table.columns else np.nan
    for p in all_progs:
        val_prog = time_table.loc[n, p]
        
        if pd.isna(val_prog) or pd.isna(val_seq):
            speedup_table.loc[n, p] = "N/A"
        elif val_seq == TIMEOUT_STR:
            if isinstance(val_prog, float):
                speedup_table.loc[n, p] = f"> {(TIMEOUT_NUM / val_prog):.2f}"
            else:
                speedup_table.loc[n, p] = "Timeout"
        elif isinstance(val_seq, float):
            if isinstance(val_prog, float):
                speedup_table.loc[n, p] = f"{(val_seq / val_prog):.2f}"
            else:
                speedup_table.loc[n, p] = "N/A"

# 5. Heuristic Accuracy Table (Baseline: exact_cuda_e)
valid_runs = df_merged[df_merged['Solution_Value'] > 0]

baseline_sols = valid_runs[valid_runs['Program'] == 'exact_cuda_e'][['Test_Case', 'Solution_Value']]
baseline_sols = baseline_sols.rename(columns={'Solution_Value': 'Exact_Value'})

acc_df = valid_runs.merge(baseline_sols, on='Test_Case', how='inner')
# Accuracy = (Optimal / Heuristic) * 100
acc_df['Accuracy_%'] = (acc_df['Exact_Value'] / acc_df['Solution_Value']) * 100.0

acc_median = acc_df.pivot_table(
    index='Vertices_N', 
    columns='Program', 
    values='Accuracy_%', 
    aggfunc=np.median
)

# Clean up accuracy table (Drop exact solvers and bugged genetic_par)
cols_to_drop = [c for c in acc_median.columns if 'exact' in c or 'genetic_par' in c]
acc_median_print = acc_median.drop(columns=cols_to_drop, errors='ignore')
acc_median_print = acc_median_print.map(lambda x: f"{x:.2f}" if pd.notnull(x) else "N/A")

# --- OUTPUT ---
print("=== MEDIAN EXECUTION TIME (Seconds) ===")
print(time_print.to_markdown())

print("\n\n=== PARALLEL SPEEDUP (Relative to exact_seq) ===")
print(speedup_table.to_markdown())

print("\n\n=== HEURISTIC ACCURACY (%) ===")
print(acc_median_print.to_markdown())