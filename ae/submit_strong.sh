#!/bin/bash
# Submits the strong-scaling actor sweep for the GAP benchmark suite:
# 5 matrices x 7 node counts x 4 configs (row_block csr/csc, column_block
# csc/csr), actor-only. Output lands under $AE_ROOT/results/strong, laid out
# to match plot_strong.py's ACTOR_VARIANTS.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AE_ROOT="$(cd "$HERE/.." && pwd)"

BIN="$AE_ROOT/src/actors/main"
DATA_DIR="/storage/project/r-vsarkar9-1/shared/GAP_dataset"
OUT_ROOT="$AE_ROOT/results/strong"
MATRICES=(GAP-kron GAP-road GAP-twitter GAP-urand GAP-web)
NODE_COUNTS=(1 2 4 8 16 32 64)
TASKS_PER_NODE=24
ACCOUNT=gts-ahayashi6
# Restrict node range for quick test runs, e.g.
# MIN_NODES=2 MAX_NODES=4 ./submit_strong.sh -> only N=2 and N=4
MIN_NODES=${MIN_NODES:-1}
MAX_NODES=${MAX_NODES:-64}

if [ ! -x "$BIN" ]; then
    echo "error: $BIN not found/executable. Build it first: $HERE/run_ae.sh build" >&2
    exit 1
fi

mkdir -p "$OUT_ROOT"

for m in "${MATRICES[@]}"; do
    MTX="$DATA_DIR/$m.mtx"
    if [ ! -f "$MTX" ]; then
        echo "warning: $MTX not found, skipping $m" >&2
        continue
    fi
    for N in "${NODE_COUNTS[@]}"; do
        if [ "$N" -gt "$MAX_NODES" ] || [ "$N" -lt "$MIN_NODES" ]; then
            continue
        fi
        NTASKS=$((TASKS_PER_NODE * N))
        sbatch --account="$ACCOUNT" --nodes="$N" --ntasks="$NTASKS" \
            --job-name="${m}_N${N}_strong" \
            "$HERE/run_strong.sbatch" "$MTX" "$OUT_ROOT" "$BIN"
    done
done
