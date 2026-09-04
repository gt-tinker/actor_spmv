#!/bin/bash
# Submits the weak-scaling actor sweep for erdos-renyi and rmat.
#
# Three "ladders" per family (scale = scale_start + k, nodes = 2^k):
#   A: scale_start=18, k=1..6  -> scales 19-24, nodes 2-64
#   B: scale_start=21, k=1..6  -> scales 22-27, nodes 2-64
#   C: scale_start=24, k=1..6  -> scales 25-30, nodes 2-64   (erdos-renyi only)
#   C: scale_start=24, k=1..5  -> scales 25-29, nodes 2-32   (rmat: no 30/64 point)
#
# Each (scale, node) pair becomes one sbatch submission x 4 configs. Output
# lands under $AE_ROOT/results/weak, laid out to match plot_weak.py.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AE_ROOT="$(cd "$HERE/.." && pwd)"

BIN="$AE_ROOT/src/actors/main"
DATA_DIR="/storage/project/r-vsarkar9-1/shared/spmv_dataset"
OUT_ROOT="$AE_ROOT/results/weak"
TASKS_PER_NODE=24
ACCOUNT=gts-ahayashi6
# Cap node count for quick test runs, e.g. MAX_NODES=16 ./submit_weak.sh
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
        if [ "$NODES" -gt "$MAX_NODES" ]; then
            continue
        fi
        local key="${family}_${SCALE}_${NODES}"
        if [ -n "${SEEN[$key]:-}" ]; then
            continue
        fi
        SEEN[$key]=1

        local NAME="${family}${sep}${SCALE}"
        local MTX="$DATA_DIR/${NAME}.mtx"
        if [ ! -f "$MTX" ]; then
            echo "warning: $MTX not found, skipping" >&2
            continue
        fi
        local NTASKS=$((TASKS_PER_NODE * NODES))
        sbatch --account="$ACCOUNT" --nodes="$NODES" --ntasks="$NTASKS" \
            --job-name="${NAME}_N${NODES}_weak" \
            "$HERE/run_weak.sbatch" "$MTX" "$OUT_ROOT" "$BIN"
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
