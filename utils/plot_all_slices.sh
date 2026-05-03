#!/usr/bin/env bash
# Run plot_slice.py for every N and every thread count used in the benchmarks.
#
# Usage:
#   ./plot_all_slices.sh [output_dir]
#
# Reads CSVs from ../results/ (matching bs2_*.csv and bs3_*.csv).
# Writes PNGs to output_dir/<algo>/ (default: ../results/plots/<algo>/).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PYTHON="$SCRIPT_DIR/.venv/bin/python"
PLOT_SCRIPT="$SCRIPT_DIR/plot_slice.py"
RESULTS_DIR="$PROJECT_DIR/results"
OUT_DIR="${1:-$RESULTS_DIR/plots}"

N_VALUES=(100 300 500 1000 2000 5000 10000 30000 50000 100000 150000 200000
          250000 300000 350000 400000 450000 500000 550000 600000 650000 700000
          725000 750000 775000 800000 825000 850000 875000 900000 925000 950000
          975000 1000000 1500000 2000000 2500000)
THREAD_VALUES=(1 2 3 4 6 8 10 12 16 20 24 32 48)

run_slices() {
    local csv="$1"
    local algo="$2"
    local algo_out="$OUT_DIR/$algo"
    mkdir -p "$algo_out"

    echo "=== $algo  ($csv)  =>  $algo_out"

    for N in "${N_VALUES[@]}"; do
        if (cd "$algo_out" && "$PYTHON" "$PLOT_SCRIPT" "$csv" "$algo" N "$N") 2>/dev/null; then
            echo "  OK    N=$N"
        else
            echo "  SKIP  N=$N  (no data)"
        fi
    done

    for T in "${THREAD_VALUES[@]}"; do
        if (cd "$algo_out" && "$PYTHON" "$PLOT_SCRIPT" "$csv" "$algo" threads "$T") 2>/dev/null; then
            echo "  OK    threads=$T"
        else
            echo "  SKIP  threads=$T  (no data)"
        fi
    done
}

for csv in "$RESULTS_DIR"/bs2_*.csv; do
    [[ -f "$csv" ]] && run_slices "$csv" "bs2"
done

for csv in "$RESULTS_DIR"/bs3_*.csv; do
    [[ -f "$csv" ]] && run_slices "$csv" "bs3"
done

echo "Done. All plots in $OUT_DIR"
