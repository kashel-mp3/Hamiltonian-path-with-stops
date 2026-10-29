#!/bin/bash

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <test_directory> <program_path> <output_file>"
    exit 1
fi

TEST_DIR="$1"
PROGRAM="$2"
OUTPUT_FILE="$3"

> "$OUTPUT_FILE"

export OMP_PROC_BIND=true
export OMP_PLACES=cores

for testcase in "$TEST_DIR"/*.json; do
    filename=$(basename "$testcase")
    output=$("$PROGRAM" "$testcase")
    echo "$filename"
    echo "$filename $output" >> "$OUTPUT_FILE"
done
