#!/bin/bash
#SBATCH --account=gts-vsarkar9-coda20        # account to which job is charged
#SBATCH --job-name=petsc-spmv         # overridden by submit.sh (--job-name)
#SBATCH -t60                        # job will run at most 120mins
#SBATCH -q inferno                   # job is submitted to inferno queue
#SBATCH --mem-per-cpu=8G
#SBATCH --partition=cpu-small
#SBATCH -N 1
#SBATCH -n 1
#SBATCH -o %x_%j.log

# Usage: sbatch --nodes=<N> --ntasks=<N> --job-name=<name> run_petsc.sbatch <matrix.petsc>
# One MPI rank per node, 1 OpenMP threads per rank (matches run.sh).
export OMP_NUM_THREADS=1

FILE=$1
GRAPHNAME=$(basename "$FILE")
GRAPHNAME=${GRAPHNAME%.*}

OUTFILE="${GRAPHNAME}_${SLURM_NNODES}_${SLURM_NTASKS}.out"
srun -N "$SLURM_NNODES" -n "$SLURM_NTASKS" --cpu-bind=core -u ./main -f "$FILE" -mat_type mpiaij -log_view > "$OUTFILE" 2>&1
