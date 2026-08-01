#!/bin/bash
# Usage: ./submit.sh <matrix.petsc>
# Submits run_petsc.sbatch once per node count N in {1,2,4,8,16,32,64},
# with one MPI rank per node (-n = N) and 24 OpenMP threads per rank.
# Job name is <graph>_N<N> so it's easy to identify in squeue.

FILE=$1

if [ -z "$FILE" ]; then
    echo "Usage: $0 <matrix.petsc>"
    exit 1
fi

GRAPH=$(basename "$FILE")
GRAPH=${GRAPH%.*}

# for N in 1 2 4 8 16 32 64; do
for N in 64; do
    NTASKS=$((N * 24))
    sbatch --nodes="$N" --ntasks="$NTASKS" --job-name="${GRAPH}_N${N}" run_petsc.sbatch "$FILE"
done
