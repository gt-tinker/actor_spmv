# export OMP_NUM_THREADS=24
DATA=$1
srun -N 1 -n 1 --cpu-bind=core -u ./main -f $DATA -mat_type mpiaij -log_view