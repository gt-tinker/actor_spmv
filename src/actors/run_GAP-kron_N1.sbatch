#!/bin/bash
#SBATCH --account=gts-vsarkar9        # account to which job is charged
#SBATCH --job-name=GAP-kron_N1        # job name: <graph>_N<nodes>
#SBATCH -t120                        # job will run at most 120mins
#SBATCH -q inferno                   # job is submitted to inferno queue
#SBATCH --mem-per-cpu=8G
#SBATCH --partition=cpu-small
#SBATCH -N 2
#SBATCH -n 48
#SBATCH -o %x_%j.out

srun -N 2 -n 48 --cpu-bind=core -u ./main -f /storage/project/r-vsarkar9-1/shared/spmv_data/data/GAP-kron/GAP-kron.mtx -d row -p block -m csr