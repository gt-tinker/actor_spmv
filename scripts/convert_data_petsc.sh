python3 -m pip install --user scipy
EXE=$PETSC_DIR/lib/petsc/bin/PetscBinaryIO.py
DATA=$1
python3 ${EXE} convert --indices 64bit --precision double ${DATA}
