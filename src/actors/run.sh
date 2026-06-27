DATA=$1
DIMENSION=$2
PARTITION=$3
FORMAT=$4
srun -N 1 -n 1 --cpu-bind=core -u ./main -f $DATA -d $DIMENSION -p $PARTITION -m $FORMAT