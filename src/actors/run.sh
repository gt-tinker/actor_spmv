
srun -N 2 -n $((24*2)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-19.mtx
# srun -N 4 -n $((24*4)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-20.mtx
# srun -N 8 -n $((24*8)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-21.mtx
# srun -N 16 -n $((24*16)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-22.mtx
# srun -N 32 -n $((24*32)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-23.mtx
# srun -N 64 -n $((24*64)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-24.mtx

# srun -N 2 -n $((24*2)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-22.mtx
# srun -N 4 -n $((24*4)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-23.mtx
# srun -N 8 -n $((24*8)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-24.mtx
# srun -N 16 -n $((24*16)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-25.mtx
# srun -N 32 -n $((24*32)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-26.mtx
# srun -N 64 -n $((24*64)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-27.mtx

# srun -N 2 -n $((24*2)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-25.mtx
# srun -N 4 -n $((24*4)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-26.mtx
# srun -N 8 -n $((24*8)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-27.mtx
# srun -N 16 -n $((24*16)) ./main -d row -p block -m csr -f ~/scratch/spmv_dataset/erdos-renyi-28.mtx