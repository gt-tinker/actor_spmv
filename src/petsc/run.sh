DATA=$1
NODES=$2
srun -N ${NODES} -n $((24*NODES)) --cpu-bind=core -c1 ./main -f $DATA -mat_type mpiaij