#!/usr/bin/env bash
#SBATCH --mail-type=BEGIN,END,FAIL
#SBATCH --mail-user=kuba.rekas.307@gmail.com
#SBATCH --job-name=omp-bs3-check
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48
#SBATCH --time=01:30:00
#SBATCH --partition=plgrid
#SBATCH --output=logs/bs3_check-%j.out
#SBATCH --error=logs/bs3_check-%j.err
#SBATCH --exclusive
#SBATCH --account=plgmpr26-cpu

set -euo pipefail

N_VALUES=(100000)
THREAD_VALUES=(1 2 8 16 48)

if [[ -n "${SLURM_SUBMIT_DIR:-}" ]]; then
    PROJECT_DIR="$SLURM_SUBMIT_DIR"
else
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
fi

RESULTS_DIR="$PROJECT_DIR/results"
mkdir -p "$RESULTS_DIR" "$PROJECT_DIR/logs"

RESULT_FILE="$RESULTS_DIR/bs3_check_result.txt"
echo "Results -> $RESULT_FILE"

if command -v module &>/dev/null; then
    module load gcc
fi

echo "Building bs3_check..."
make -C "$PROJECT_DIR" bs3_check

export OMP_PLACES=cores
export OMP_PROC_BIND=close

MAX_THREADS="${SLURM_CPUS_PER_TASK:-128}"

TOTAL=0
SKIPPED=0
FAILED=0

for N in "${N_VALUES[@]}"; do
    for T in "${THREAD_VALUES[@]}"; do
        if (( T > N )); then
            echo "  SKIP  bs3_check N=$N threads=$T (threads > N)"
            (( SKIPPED++ )) || true
            continue
        fi
        if (( T > MAX_THREADS )); then
            echo "  SKIP  bs3_check N=$N threads=$T (threads > allocated $MAX_THREADS)"
            (( SKIPPED++ )) || true
            continue
        fi
        echo "  RUN   bs3_check N=$N threads=$T"
        if ! OMP_NUM_THREADS="$T" "$PROJECT_DIR/out/bs3_check" "$N" >> "$RESULT_FILE"; then
            echo "  WARN  bs3_check N=$N threads=$T failed — row omitted"
            (( FAILED++ )) || true
        fi
        (( TOTAL++ )) || true
    done
done

echo "Done. $TOTAL runs, $SKIPPED skipped, $FAILED failed."
echo "Results saved to $RESULT_FILE"
