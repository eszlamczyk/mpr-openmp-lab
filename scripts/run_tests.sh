#!/bin/bash

set -euo pipefail

N_VALUES=(100 300 500 1000 2000 5000 10000 30000 50000 100000 150000 200000 250000 300000 350000 400000 450000 500000 550000 600000 650000 700000 725000 750000 775000 800000 825000 850000 875000 900000 925000 950000 975000 1000000)
THREAD_VALUES=(1 2 3 4 6 8 10 12 16 20 24 32 48 64 96 128)
VARIANTS=(bs2 bs3)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

OUTPUT_NAME="${1:-results}"
RESULTS_DIR="$PROJECT_DIR/results"
mkdir -p "$RESULTS_DIR"

echo "Building bs2 and bs3 with CSV=1..."
make -C "$PROJECT_DIR" bs2 CSV=1
make -C "$PROJECT_DIR" bs3 CSV=1

OUTPUT_FILE="$RESULTS_DIR/${OUTPUT_NAME}.csv"
echo "Results -> $OUTPUT_FILE"
> "$OUTPUT_FILE"

for VARIANT in "${VARIANTS[@]}"; do
    for N in "${N_VALUES[@]}"; do
        for T in "${THREAD_VALUES[@]}"; do
            if (( T > N )); then
                echo "  $VARIANT N=$N threads=$T (skipped: threads > N)"
                continue
            fi
            echo "  $VARIANT N=$N threads=$T"
            OMP_NUM_THREADS="$T" "$PROJECT_DIR/out/$VARIANT" "$N" >> "$OUTPUT_FILE" || echo "  $VARIANT N=$N threads=$T FAILED (skipped)"
        done
    done
done

echo "Done. Results saved to $OUTPUT_FILE"
