#!/usr/bin/env bash
# Run plot_slice.py for every N and every thread count used in the benchmarks.
#
# Usage:
#   ./plot_all_slices.sh [output_dir]
#
# Reads CSVs from ../results/ (matching bs2_*.csv and bs3_*.csv).
# Writes PNGs to output_dir/<csv_name>/ (default: ../results/plots/<csv_name>/).
# Parallelism defaults to nproc; override with JOBS=N.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PYTHON="$PROJECT_DIR/.venv/bin/python"
PLOT_SCRIPT="$SCRIPT_DIR/plot_slice.py"
RESULTS_DIR="$PROJECT_DIR/results"
OUT_DIR="${1:-$RESULTS_DIR/plots}"
MAX_JOBS="${JOBS:-$(nproc)}"

N_VALUES=(100 300 500 1000 2000 5000 10000 30000 50000 100000 150000 200000
          250000 300000 350000 400000 450000 500000 550000 600000 650000 700000
          725000 750000 775000 800000 825000 850000 875000 900000 925000 950000
          975000 1000000 1500000 2000000 2500000)
THREAD_VALUES=(1 2 3 4 6 8 10 12 16 20 24 32 48)

run_one() {
    local algo_out="$1" csv="$2" algo="$3" ftype="$4" value="$5"
    if (cd "$algo_out" && "$PYTHON" "$PLOT_SCRIPT" "$csv" "$algo" "$ftype" "$value") 2>/dev/null; then
        echo "  OK    $ftype=$value"
    else
        echo "  SKIP  $ftype=$value  (no data)"
    fi
}

throttle() {
    while (( $(jobs -rp | wc -l) >= MAX_JOBS )); do
        wait -n 2>/dev/null || true
    done
}

run_slices() {
    local csv="$1"
    local algo="$2"
    local csv_name
    csv_name="$(basename "$csv" .csv)"
    local algo_out="$OUT_DIR/$csv_name"
    mkdir -p "$algo_out"

    echo "=== $algo  ($csv)  =>  $algo_out"

    for N in "${N_VALUES[@]}"; do
        throttle
        run_one "$algo_out" "$csv" "$algo" N "$N" &
    done

    for T in "${THREAD_VALUES[@]}"; do
        throttle
        run_one "$algo_out" "$csv" "$algo" threads "$T" &
    done

    wait
}

for csv in "$RESULTS_DIR"/bs2_*.csv; do
    [[ -f "$csv" ]] && run_slices "$csv" "bs2"
done

for csv in "$RESULTS_DIR"/bs3_*.csv; do
    [[ -f "$csv" ]] && run_slices "$csv" "bs3"
done

echo "Done. All plots in $OUT_DIR"
