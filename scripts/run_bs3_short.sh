#!/usr/bin/env bash
#SBATCH --job-name=omp-bs3-short
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64
#SBATCH --time=00:10:00
#SBATCH --partition=plgrid-short
#SBATCH --output=logs/bs3-short-%j.out
#SBATCH --error=logs/bs3-short-%j.err
#SBATCH --exclusive
##SBATCH --account=<your-grant-id>

set -euo pipefail

N_VALUES=(1000 10000 100000)
THREAD_VALUES=(1 4 16 64 128)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

if [[ -n "${SLURM_JOB_ID:-}" ]]; then
    OUTPUT_NAME="${1:-bs3_short_${SLURM_JOB_ID}}"
else
    OUTPUT_NAME="${1:-bs3_short}"
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
FAILED=0

for N in "${N_VALUES[@]}"; do
    for T in "${THREAD_VALUES[@]}"; do
        if (( T > N || T > MAX_THREADS )); then
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

echo "Done. $TOTAL runs, $FAILED failed."
echo "Results saved to $OUTPUT_FILE"
