# Sparse Matrix Dense Vector Multiplication (SpMV)
This repository consists of SpMV kernels built on PETSc and HClib Actor runtimes. The programs are constructed to showcase the effectiveness of FA-BSP programming model implemented using HClib Actors. 

# Directory Structure
```tree
├── README.md
├── scripts
│   ├── convert_data_petsc.sh
│   ├── hclib_install.sh
│   └── petsc_install.sh
├── src
│   ├── actors
│   └── petsc
│       ├── Makefile
│       └── main.cpp
└── test
    ├── data.mtx
    ├── data.petsc
    └── test_petsc_data.cpp

5 directories, 10 files
```

# Installation and Builds of Programs
## HClib Actor program
Download and install the HClib Actor runtime using,
```bash
cd actor_spmv/scripts/
source hclib_install.sh
```
> If you have already downloaded the runtime before, still follow the command to activate the environment variables and set the correct library paths in your system. 

Next, we will compile the HClib Actor program and generate the executable. 
```bash
cd actor_spmv/src/actors
make
```

## PETSc program
Download and install the PETSc runtime using,
```bash
cd actor_spmv/scripts/
source petsc_install.sh
```
> If you have already downloaded the runtime before, still follow the command to activate the environment variables and set the correct library paths in your system. 

Next, we will compile the PETSc program and generate the executable. 
```bash
cd actor_spmv/src/petsc
make
```

# Dataset Generation
This step assumes that you have downloaded the GAP-benchmark files from SuiteSparse which are of the extension `.mtx`. We recommend using scratch space for your dataset for higher IOPS, although it is not a mandatory requisite.

HClib Actor program can simply use this Matrix Market Format off the shelf. However, PETSc requires a special binary format. To generate the `.petsc` format, do:
```bash
python3 actor_spmv/scripts/convert_data_petsc.sh <path/to/.mtx>
```

This command will generate an equivalent file with `.petsc` extension. It reads the header of MTX to understand the file structure. If you want to confirm that your `.petsc` is built correctly or not, follow these steps: 
```bash
cd actor_spmv/test
mpicxx -o test_petsc_data test_petsc_data.cpp -I$PETSC_DIR/include -I$PETSC_DIR/$PETSC_ARCH/include -L$PETSC_DIR/$PETSC_ARCH/lib -lpetsc
srun -N 1 -n 1 ./test_petsc_data -f <path/to/.petsc>
```

This will output the graph in adjacency list format which is easy to confirm whether it matches your intial `.mtx` file or not. 

If this program returns segmentation fault or doesn't match your expected output, please use the right header in your MTX file. This is not expected behavior for SuiteSparse graphs. This only applies for ill-generated `.mtx` files whose headers are inconsistent, e.g. using pattern instead of real while having weights. 

> This step from the beginning assumes that all environment variables for PETSc and HClib are loaded properly. If not, please refer to the [Installation](#installation-and-builds-of-programs) step again!

# Execution of Programs
