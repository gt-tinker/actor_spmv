#!/bin/bash
#SBATCH --account=gts-vsarkar9-coda20       # account to which job is charged
#SBATCH --job-name=spmv               # overridden by submit.sh (--job-name)
#SBATCH -t60                        # job will run at most 120mins
#SBATCH -q inferno                   # job is submitted to inferno queue
#SBATCH --mem-per-cpu=8G
#SBATCH --partition=cpu-small
#SBATCH -N 1
#SBATCH -n 1
#SBATCH -o %x_%j.log

# Usage: sbatch --nodes=<N> --ntasks=<n> --job-name=<name> run_configs.sbatch <graph.mtx>
FILE=$1
GRAPHNAME=$(basename "$FILE")
GRAPHNAME=${GRAPHNAME%.*}

# (dimension, partition, format-flag, actual format used). The -m flag is
# csr/csc only; main.cpp picks DCSC/DCSR internally based on the
# dimension+format pair (see main.cpp:109-137).
CONFIGS=(
    "row    cyclic csr CSR"
    "row    block  csr CSR"
    "row    cyclic csc DCSC"
    "row    block  csc DCSC"
    "column cyclic csc CSC"
    "column block  csc CSC"
    "column cyclic csr DCSR"
    "column block  csr DCSR"
)

for cfg in "${CONFIGS[@]}"; do
    read -r DIMENSION PARTITION FORMAT FORMATUSED <<< "$cfg"
    OUTFILE="${GRAPHNAME}_${SLURM_NNODES}_${SLURM_NTASKS}_${DIMENSION}_${PARTITION}_${FORMAT}_${FORMATUSED}.out"
    srun -N "$SLURM_NNODES" -n "$SLURM_NTASKS" ./main -f "$FILE" -d "$DIMENSION" -p "$PARTITION" -m "$FORMAT" > "$OUTFILE" 2>&1
done
