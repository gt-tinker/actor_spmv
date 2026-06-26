//mpicxx -o test_petsc_data test_petsc_data.cpp -I$PETSC_DIR/include -I$PETSC_DIR/$PETSC_ARCH/include -L$PETSC_DIR/$PETSC_ARCH/lib -lpetsc
#include <petscsys.h>
#include <petscmat.h>

int main(int argc, char **argv)
{
  Mat       A;
  PetscInt  M, N, i, ncols;
  char      filename[PETSC_MAX_PATH_LEN];
  PetscBool flg;

  PetscCall(PetscInitialize(&argc, &argv, NULL, NULL));

  PetscCall(PetscOptionsGetString(NULL, NULL, "-f", filename, sizeof(filename), &flg));
  if (!flg) {
    PetscPrintf(PETSC_COMM_SELF, "Usage: %s -f <matrix.bin>\n", argv[0]);
    PetscCall(PetscFinalize());
    return 1;
  }

  PetscViewer viewer;
  PetscCall(PetscViewerBinaryOpen(PETSC_COMM_SELF, filename, FILE_MODE_READ, &viewer));
  PetscCall(MatCreate(PETSC_COMM_SELF, &A));
  PetscCall(MatSetFromOptions(A));
  PetscCall(MatLoad(A, viewer));
  PetscCall(PetscViewerDestroy(&viewer));

  PetscCall(MatGetSize(A, &M, &N));
  PetscPrintf(PETSC_COMM_SELF, "Matrix: %ld x %ld\n\n", (long)M, (long)N);

  for (i = 0; i < M; i++) {
    const PetscInt    *cols;
    const PetscScalar *vals;
    PetscCall(MatGetRow(A, i, &ncols, &cols, &vals));
    for (PetscInt j = 0; j < ncols; j++) {
      PetscPrintf(PETSC_COMM_SELF, "%8ld  %8ld  %14.6g\n", (long)i, (long)cols[j], (double)vals[j]);
    }
    PetscCall(MatRestoreRow(A, i, &ncols, &cols, &vals));
  }

  PetscCall(MatDestroy(&A));
  PetscCall(PetscFinalize());
  return 0;
}