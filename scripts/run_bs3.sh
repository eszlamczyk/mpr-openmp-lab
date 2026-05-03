#!/usr/bin/env bash
#SBATCH --mail-type=BEGIN,END,FAIL
#SBATCH --mail-user=kuba.rekas.307@gmail.com
#SBATCH --mail-user=eszlamczyk@student.agh.edu.pl
#SBATCH --job-name=omp-bs3
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=128
#SBATCH --time=01:30:00
#SBATCH --partition=plgrid
#SBATCH --output=logs/bs3-%j.out
#SBATCH --error=logs/bs3-%j.err
#SBATCH --exclusive
#SBATCH --account=plgmpr26-cpu

set -euo pipefail

N_VALUES=(100 300 500 1000 2000 5000 10000 30000 50000 100000 150000 200000 250000 300000 350000 400000 450000 500000 550000 600000 650000 700000 725000 750000 775000 800000 825000 850000 875000 900000 925000 950000 975000 1000000)
THREAD_VALUES=(1 2 3 4 6 8 10 12 16 20 24 32 48 64 96 128)

if [[ -n "${SLURM_SUBMIT_DIR:-}" ]]; then
    PROJECT_DIR="$SLURM_SUBMIT_DIR"
else
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
fi

if [[ -n "${SLURM_JOB_ID:-}" ]]; then
    OUTPUT_NAME="${1:-bs3_${SLURM_JOB_ID}}"
else
    OUTPUT_NAME="${1:-bs3}"
fi

RESULTS_DIR="$PROJECT_DIR/results"
mkdir -p "$RESULTS_DIR" "$PROJECT_DIR/logs"

OUTPUT_FILE="$RESULTS_DIR/${OUTPUT_NAME}.csv"
echo "Results -> $OUTPUT_FILE"

if command -v module &>/dev/null; then
    module load gcc
fi

echo "Building bs3..."
make -C "$PROJECT_DIR" bs3

echo "algorithm,n_threads,n_elems,t_random,t_distribute,t_merge,t_sort,t_writeback,t_total,sorted" \
    > "$OUTPUT_FILE"

export OMP_PLACES=cores
export OMP_PROC_BIND=close

MAX_THREADS="${SLURM_CPUS_PER_TASK:-128}"

TOTAL=0
SKIPPED=0
FAILED=0

for N in "${N_VALUES[@]}"; do
    for T in "${THREAD_VALUES[@]}"; do
        if (( T > N )); then
            echo "  SKIP  bs3 N=$N threads=$T (threads > N)"
            (( SKIPPED++ )) || true
            continue
        fi
        if (( T > MAX_THREADS )); then
            echo "  SKIP  bs3 N=$N threads=$T (threads > allocated $MAX_THREADS)"
            (( SKIPPED++ )) || true
            continue
        fi
        echo "  RUN   bs3 N=$N threads=$T"
        if ! OMP_NUM_THREADS="$T" "$PROJECT_DIR/out/bs3" "$N" "$OUTPUT_FILE"; then
            echo "  WARN  bs3 N=$N threads=$T failed — row omitted"
            (( FAILED++ )) || true
        fi
        (( TOTAL++ )) || true
    done
done

echo "Done. $TOTAL runs, $SKIPPED skipped, $FAILED failed."
echo "Results saved to $OUTPUT_FILE"
