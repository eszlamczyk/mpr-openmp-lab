#!/usr/bin/env bash
#SBATCH --mail-type=BEGIN,END,FAIL
#SBATCH --mail-user=kuba.rekas.307@gmail.com
#SBATCH --job-name=omp-scheduler
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48
#SBATCH --time=01:00:00
#SBATCH --partition=plgrid
#SBATCH --output=logs/scheduler-%j.out
#SBATCH --error=logs/scheduler-%j.err
#SBATCH --exclusive
#SBATCH --account=plgmpr26-cpu

set -euo pipefail

N=10000000
THREAD_VALUES=(1 2 8 16 32 48)

if [[ -n "${SLURM_SUBMIT_DIR:-}" ]]; then
    PROJECT_DIR="$SLURM_SUBMIT_DIR"
else
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
fi

if [[ -n "${SLURM_JOB_ID:-}" ]]; then
    OUTPUT_NAME="${1:-scheduler_${SLURM_JOB_ID}}"
else
    OUTPUT_NAME="${1:-scheduler}"
fi

RESULTS_DIR="$PROJECT_DIR/results"
mkdir -p "$RESULTS_DIR" "$PROJECT_DIR/logs"

OUTPUT_FILE="$RESULTS_DIR/${OUTPUT_NAME}.csv"
echo "Results -> $OUTPUT_FILE"

if command -v module &>/dev/null; then
    module load gcc
fi

echo "Building scheduler..."
make -C "$PROJECT_DIR" scheduler

export OMP_PLACES=cores
export OMP_PROC_BIND=close

MAX_THREADS="${SLURM_CPUS_PER_TASK:-128}"

TOTAL=0
SKIPPED=0
FAILED=0

for T in "${THREAD_VALUES[@]}"; do
    if (( T > MAX_THREADS )); then
        echo "  SKIP  scheduler threads=$T (threads > allocated $MAX_THREADS)"
        (( SKIPPED++ )) || true
        continue
    fi
    echo "  RUN   scheduler N=$N threads=$T"
    if ! OMP_NUM_THREADS="$T" "$PROJECT_DIR/out/scheduler" "$N" "$OUTPUT_FILE"; then
        echo "  WARN  scheduler N=$N threads=$T failed — row omitted"
        (( FAILED++ )) || true
    fi
    (( TOTAL++ )) || true
done

echo "Done. $TOTAL runs, $SKIPPED skipped, $FAILED failed."
echo "Results saved to $OUTPUT_FILE"
