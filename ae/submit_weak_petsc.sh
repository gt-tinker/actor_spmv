#!/bin/bash
# Submits the weak-scaling PETSc sweep for erdos-renyi and rmat: MPI-only
# PETSc (OpenMP disabled at build time, 24 ranks/node, OMP_NUM_THREADS=1).
# Same three "ladders" per family as the actor sweep (see submit_weak.sh),
# reading the pre-converted .petsc binary files already sitting next to
# each .mtx in DATA_DIR.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AE_ROOT="$(cd "$HERE/.." && pwd)"

BIN="$AE_ROOT/src/petsc/main"
DATA_DIR="/storage/project/r-vsarkar9-1/shared/spmv_dataset"
OUT_ROOT="$AE_ROOT/results/weak"
TASKS_PER_NODE=24
ACCOUNT=gts-ahayashi6
# Restrict node range for quick test runs, e.g.
# MIN_NODES=2 MAX_NODES=4 ./submit_weak_petsc.sh -> only N=2 and N=4
MIN_NODES=${MIN_NODES:-1}
MAX_NODES=${MAX_NODES:-64}

if [ ! -x "$BIN" ]; then
    echo "error: $BIN not found/executable. Build it first: $HERE/run_ae.sh build" >&2
    exit 1
fi

mkdir -p "$OUT_ROOT"

# Emits "<scale> <nodes>" lines for k=1..max_k (nodes=2^k, scale=scale_start+k).
ladder() {
    local scale_start=$1 max_k=$2
    for ((k = 1; k <= max_k; k++)); do
        echo "$((scale_start + k)) $((2 ** k))"
    done
}

declare -A SEEN

submit_family() {
    local family=$1 sep=$2
    shift 2
    local pair
    for pair in "$@"; do
        read -r SCALE NODES <<< "$pair"
        if [ "$NODES" -gt "$MAX_NODES" ] || [ "$NODES" -lt "$MIN_NODES" ]; then
            continue
        fi
        local key="${family}_${SCALE}_${NODES}"
        if [ -n "${SEEN[$key]:-}" ]; then
            continue
        fi
        SEEN[$key]=1

        local NAME="${family}${sep}${SCALE}"
        local MTX="$DATA_DIR/${NAME}.petsc"
        if [ ! -f "$MTX" ]; then
            echo "warning: $MTX not found, skipping" >&2
            continue
        fi
        local NTASKS=$((TASKS_PER_NODE * NODES))
        sbatch --account="$ACCOUNT" --nodes="$NODES" --ntasks="$NTASKS" \
            --job-name="${NAME}_N${NODES}_weak_petsc" \
            "$HERE/run_weak_petsc.sbatch" "$MTX" "$OUT_ROOT" "$BIN"
    done
}

mapfile -t ERDOS_A < <(ladder 18 6)
mapfile -t ERDOS_B < <(ladder 21 6)
mapfile -t ERDOS_C < <(ladder 24 6)
submit_family "erdos-renyi" "-" "${ERDOS_A[@]}" "${ERDOS_B[@]}" "${ERDOS_C[@]}"

mapfile -t RMAT_A < <(ladder 18 6)
mapfile -t RMAT_B < <(ladder 21 6)
mapfile -t RMAT_C < <(ladder 24 5)
submit_family "rmat" "_" "${RMAT_A[@]}" "${RMAT_B[@]}" "${RMAT_C[@]}"
