#!/bin/bash
# Usage: ./submit.sh <graph.mtx>
# Submits run_configs.sbatch once per node count N in {1,2,4,8,16,32,64},
# with -n always 24*N. Job name is <graph>_N<N> so it's easy to identify
# in squeue.

FILE=$1

if [ -z "$FILE" ]; then
    echo "Usage: $0 <graph.mtx>"
    exit 1
fi

GRAPH=$(basename "$FILE")
GRAPH=${GRAPH%.*}

# for N in 1 2 4 8 16 32 64; do
for N in 4; do
    NTASKS=$((24 * N))
    sbatch --nodes="$N" --ntasks="$NTASKS" --job-name="${GRAPH}_N${N}" run_configs.sbatch "$FILE"
done
