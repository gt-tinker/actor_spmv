module load gcc python/3.12.5 openmpi/4.1.5 openblas

if [ ! -d petsc ]; then
    git clone -b release https://gitlab.com/petsc/petsc.git petsc
    cd petsc/
    ./configure --with-64-bit-indices=true --with-debugging=no --with-batch --with-openmp=0 --known-64-bit-blas-indices --known-snrm2-returns-double=1 --known-sdot-returns-double=1
    make PETSC_DIR=${PWD} PETSC_ARCH=arch-linux-c-opt all
    cd ..
fi

export PETSC_ARCH=arch-linux-c-opt
export PETSC_DIR=${PWD}/petsc/
export LD_LIBRARY_PATH=$PETSC_DIR/$PETSC_ARCH/lib:$LD_LIBRARY_PATH