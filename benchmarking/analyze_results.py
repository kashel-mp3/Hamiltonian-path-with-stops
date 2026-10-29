"""Combine benchmark result CSVs in memory and report the result tables.

Usage:
    python benchmarking/analyze_results.py [results_1.csv results_2.csv ...]

Any number of result files may be passed; they are concatenated in memory only and
nothing is written back except the summary tables in ``plots/``. Duplicate
(solver, instance) pairs across files are resolved by keeping a solved run over a
timed-out one, then the slowest run, so censored data is never hidden.
"""
import os
import sys

import numpy as np
import pandas as pd

TIMEOUT_NUM = 600.0
TIMEOUT_STR = ">600.0"

# Ground truth for accuracy: GPU edge-split solver first, sequential exact as fallback.
BASELINE_PREFERENCE = ("exact_cuda_e", "exact_seq")
# Speedups are always reported against the sequential exact solver.
SPEEDUP_BASELINE = "exact_seq"

# Excluded from accuracy: exact solvers are the baseline themselves, and genetic_par
# has known thread-safety faults (see README).
ACCURACY_EXCLUDE = {"genetic_par"}

DEFAULT_INPUTS = ("benchmark_results_20260929_120837.csv",)
REQUIRED_COLUMNS = {
    "Program",
    "Test_Case",
    "Vertices_N",
    "Solution_Value",
    "Execution_Time_s",
}
OUTPUT_DIR = "plots"


def clean_program_name(value):
    """'build/bin/Release/exact_seq.exe' -> 'exact_seq'."""
    return os.path.basename(str(value).replace("\\", "/")).replace(".exe", "")


def load_runs(paths):
    """Read and concatenate result files, one row per (solver, instance)."""
    frames = []
    for path in paths:
        if not os.path.exists(path):
            sys.exit(f"Error: could not find '{path}'")
        frame = pd.read_csv(path)
        missing = REQUIRED_COLUMNS - set(frame.columns)
        if missing:
            sys.exit(f"Error: '{path}' is missing columns: {', '.join(sorted(missing))}")
        frames.append(frame)

    runs = pd.concat(frames, ignore_index=True)
    runs["Program"] = runs["Program"].map(clean_program_name)

    runs["solved"] = runs["Solution_Value"] > 0
    runs = runs.sort_values(["solved", "Execution_Time_s"], ascending=[False, False])
    runs = runs.drop_duplicates(subset=["Program", "Test_Case"], keep="first")
    return runs.drop(columns=["solved"])


def median_time_table(runs):
    """Median execution time per (|V|, solver); unsolved configurations become timeouts."""
    node_counts = sorted(runs["Vertices_N"].unique())
    programs = sorted(runs["Program"].unique())
    table = pd.DataFrame(index=node_counts, columns=programs, dtype=object)

    for n in node_counts:
        for program in programs:
            subset = runs[(runs["Vertices_N"] == n) & (runs["Program"] == program)]
            if subset.empty:
                continue
            solved = subset[subset["Solution_Value"] > 0]
            table.loc[n, program] = (
                float(np.median(solved["Execution_Time_s"])) if not solved.empty else TIMEOUT_STR
            )
    return table


def speedup_table(times, baseline):
    """Speedup against `baseline`; censored baselines yield strict lower bounds."""
    if baseline not in times.columns:
        return pd.DataFrame()

    speedups = pd.DataFrame(index=times.index, columns=times.columns, dtype=object)
    for n in times.index:
        base = times.loc[n, baseline]
        for program in times.columns:
            value = times.loc[n, program]
            if pd.isna(base) or pd.isna(value):
                speedups.loc[n, program] = "N/A"
            elif base == TIMEOUT_STR:
                if isinstance(value, float):
                    speedups.loc[n, program] = f"> {TIMEOUT_NUM / value:.2f}"
                else:
                    speedups.loc[n, program] = "Timeout"
            elif isinstance(value, float):
                speedups.loc[n, program] = f"{base / value:.2f}"
            else:
                speedups.loc[n, program] = "N/A"
    return speedups


def accuracy_table(runs):
    """Heuristic solution quality relative to the ground-truth solver, in percent."""
    solved = runs[runs["Solution_Value"] > 0]
    baseline = next((b for b in BASELINE_PREFERENCE if b in set(solved["Program"])), None)
    if baseline is None:
        return pd.DataFrame()

    truth = solved[solved["Program"] == baseline][["Test_Case", "Solution_Value"]]
    truth = truth.rename(columns={"Solution_Value": "Exact_Value"})

    compared = solved.merge(truth, on="Test_Case", how="inner")
    compared = compared[~compared["Program"].str.startswith("exact")]
    compared = compared[~compared["Program"].isin(ACCURACY_EXCLUDE)]
    compared["Accuracy_%"] = compared["Exact_Value"] / compared["Solution_Value"] * 100.0

    return compared.pivot_table(
        index="Vertices_N", columns="Program", values="Accuracy_%", aggfunc=np.median
    ).round(2)


def format_times(times):
    formatted = times.copy()
    for column in formatted.columns:
        formatted[column] = formatted[column].map(
            lambda value: f"{value:.6g}" if isinstance(value, float) else str(value)
        )
    return formatted


def main():
    paths = sys.argv[1:] or list(DEFAULT_INPUTS)
    runs = load_runs(paths)
    baseline = next((b for b in BASELINE_PREFERENCE if b in set(runs["Program"])), None)

    times = median_time_table(runs)
    speedups = speedup_table(times, SPEEDUP_BASELINE)
    accuracy = accuracy_table(runs)

    print(f"Inputs: {', '.join(paths)}")
    print(
        f"Instances: {runs['Test_Case'].nunique()} | Solvers: {runs['Program'].nunique()}"
        f" | Accuracy baseline: {baseline or 'none'}"
    )

    print("\n=== MEDIAN EXECUTION TIME (Seconds) ===")
    print(format_times(times).to_markdown())

    print("\n\n=== PARALLEL SPEEDUP (Relative to exact_seq) ===")
    print(speedups.to_markdown())

    print("\n\n=== HEURISTIC ACCURACY (%) ===")
    print(accuracy.to_markdown())

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    times.to_csv(os.path.join(OUTPUT_DIR, "analysis_time.csv"))
    speedups.to_csv(os.path.join(OUTPUT_DIR, "analysis_speedup.csv"))
    accuracy.to_csv(os.path.join(OUTPUT_DIR, "analysis_accuracy.csv"))
    print(f"\n[+] Summaries exported as CSVs to '{OUTPUT_DIR}/'.")


if __name__ == "__main__":
    main()
