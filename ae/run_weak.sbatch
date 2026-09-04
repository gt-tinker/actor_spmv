#!/bin/bash
#SBATCH --job-name=spmv_weak
#SBATCH -t60
#SBATCH -q inferno
#SBATCH --mem-per-cpu=8G
#SBATCH --partition=cpu-small
#SBATCH -N 1
#SBATCH -n 1
#SBATCH -o %x_%j.log
#SBATCH --account=gts-ahayashi6

# Weak-scaling actor sweep: fixed (scale, node-count) pair, 4 block-partition
# configs (row_block csr/csc, column_block csc/csr). Actor-only, no PETSc.
#
# Usage: sbatch --nodes=<N> --ntasks=<n> --job-name=<name> run_weak.sbatch <graph.mtx> <out_root> <bin>
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
    OUTDIR="${OUTROOT}/output_weak_${DIMENSION}_${PARTITION}_${FORMAT}"
    mkdir -p "$OUTDIR"
    OUTFILE="${OUTDIR}/out_spmv_${GRAPHNAME}_N${SLURM_NNODES}_n${SLURM_NTASKS}"
    srun -N "$SLURM_NNODES" -n "$SLURM_NTASKS" "$BIN" -f "$FILE" -d "$DIMENSION" -p "$PARTITION" -m "$FORMAT" > "$OUTFILE" 2>&1
done
