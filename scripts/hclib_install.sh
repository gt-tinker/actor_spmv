#!/bin/bash
module load gcc python/3.12.5 openmpi/4.1.5

export CC=oshcc
export CXX=oshc++

if [ ! -d bale ]; then
    git clone https://github.com/jdevinney/bale.git
    cd bale/
    cd src/bale_classic/
    ./bootstrap.sh
    PLATFORM=oshmem ./make_bale -s -f
    cd ../../../
fi

export CC=oshcc
export CXX=oshc++

if [ ! -d hclib ]; then
    git clone https://github.com/srirajpaul/hclib
    cd hclib
    git fetch && git checkout bale3_actor
    ./install.sh
    source hclib-install/bin/hclib_setup_env.sh
    cd modules/bale_actor && make
    cd benchmarks
    unzip ../inc/boost.zip -d ../inc/
    cd ../../../../
fi

export BALE_INSTALL=$PWD/bale/src/bale_classic/build_oshmem
export HCLIB_ROOT=$PWD/hclib/hclib-install
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BALE_INSTALL/lib:$HCLIB_ROOT/lib:$HCLIB_ROOT/../modules/bale_actor/lib
export HCLIB_WORKERS=1