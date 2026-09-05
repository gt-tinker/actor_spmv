#!/bin/bash
#SBATCH --job-name=spmv_strong
#SBATCH -t120
#SBATCH -q inferno
#SBATCH --mem-per-cpu=8G
#SBATCH --partition=cpu-small
#SBATCH -N 1
#SBATCH -n 1
#SBATCH -o %x_%j.log
#SBATCH --account=gts-ahayashi6

# Strong-scaling actor sweep: fixed matrix, 4 block-partition configs
# (row_block csr/csc, column_block csc/csr -> CSR/DCSC/CSC/DCSR per
# src/actors/main.cpp's dimension+format mapping). Actor-only, no PETSc.
#
# Usage: sbatch --nodes=<N> --ntasks=<n> --job-name=<name> run_strong.sbatch <graph.mtx> <out_root> <bin>
FILE=$1
OUTROOT=$2
BIN=$3

GRAPHNAME=$(basename "$FILE")
GRAPHNAME=${GRAPHNAME%.*}

CONFIGS=(
    "row    block  csr CSR"
    "row    block  csc DCSC"
    "column block  csc CSC"
    "column block  csr DCSR"
)

for cfg in "${CONFIGS[@]}"; do
    read -r DIMENSION PARTITION FORMAT FORMATUSED <<< "$cfg"
    OUTDIR="${OUTROOT}/output_strong_${DIMENSION}_${PARTITION}_${FORMAT}/out_spmv_${GRAPHNAME}"
    mkdir -p "$OUTDIR"
    OUTFILE="${OUTDIR}/out_spmv_${GRAPHNAME}_N${SLURM_NNODES}_n${SLURM_NTASKS}"
    srun -N "$SLURM_NNODES" -n "$SLURM_NTASKS" "$BIN" -f "$FILE" -d "$DIMENSION" -p "$PARTITION" -m "$FORMAT" > "$OUTFILE" 2>&1
done
