#!/bin/bash
#SBATCH --job-name=spmv_weak_petsc
#SBATCH -t120
#SBATCH -q inferno
#SBATCH --mem-per-cpu=8G
#SBATCH --partition=cpu-small
#SBATCH -N 1
#SBATCH -n 1
#SBATCH -o %x_%j.log
#SBATCH --account=gts-ahayashi6

# Weak-scaling PETSc sweep: fixed (scale, node-count) pair, MPI-only
# (OpenMP disabled at build time, OMP_NUM_THREADS=1), 24 ranks/node.
#
# Usage: sbatch --nodes=<N> --ntasks=<n> --job-name=<name> run_weak_petsc.sbatch <graph.petsc> <out_root> <bin>
FILE=$1
OUTROOT=$2
BIN=$3

GRAPHNAME=$(basename "$FILE")
GRAPHNAME=${GRAPHNAME%.*}

export OMP_NUM_THREADS=1

OUTDIR="${OUTROOT}/output_weak_petsc"
mkdir -p "$OUTDIR"
OUTFILE="${OUTDIR}/out_spmv_${GRAPHNAME}_N${SLURM_NNODES}_n${SLURM_NTASKS}"
srun -N "$SLURM_NNODES" -n "$SLURM_NTASKS" --cpu-bind=core "$BIN" -f "$FILE" -mat_type mpiaij > "$OUTFILE" 2>&1
