module load gcc python/3.12.5 openmpi/4.1.5 openblas

# Built under ~/scratch, not here: the PETSc source+build easily runs to a
# few hundred MB-2GB, and the home filesystem quota (20GB) has no headroom
# for that (it's already close to its hard limit from other projects).
# ~/scratch is a symlink to /storage/scratch1/<...>, a separate filesystem
# with effectively unlimited space -- same place GAP-web.mtx's override lives.
PETSC_BUILD_ROOT="$HOME/scratch/ae_petsc_build"
mkdir -p "$PETSC_BUILD_ROOT"

if [ ! -d "$PETSC_BUILD_ROOT/petsc" ]; then
    git clone -b release https://gitlab.com/petsc/petsc.git "$PETSC_BUILD_ROOT/petsc"
    cd "$PETSC_BUILD_ROOT/petsc"
    ./configure --with-64-bit-indices=true --with-debugging=no --with-batch --with-openmp=0 --known-64-bit-blas-indices --known-snrm2-returns-double=1 --known-sdot-returns-double=1
    make PETSC_DIR=${PWD} PETSC_ARCH=arch-linux-c-opt all
    cd - > /dev/null
fi

export PETSC_ARCH=arch-linux-c-opt
export PETSC_DIR=$PETSC_BUILD_ROOT/petsc
export LD_LIBRARY_PATH=$PETSC_DIR/$PETSC_ARCH/lib:$LD_LIBRARY_PATH
