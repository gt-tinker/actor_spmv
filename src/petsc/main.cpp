#include <petscsys.h>
#include <petscmat.h>
#include <petscvec.h>
#include <mpi.h>
#include <stdlib.h>

#define NUM_RUNS 10

int main(int argc, char **argv)
{
  Mat            A;
  Vec            x, y;
  PetscInt       M, N;
  PetscInt       Istart, Iend, i;
  PetscMPIInt    rank;
  PetscReal      norm;
  char           filename[PETSC_MAX_PATH_LEN];
  PetscBool      flg;
  PetscErrorCode ierr;

  ierr = PetscInitialize(&argc, &argv, NULL, NULL); if (ierr) return ierr;
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD, &rank); PetscCall(ierr);

  PetscCall(PetscOptionsGetString(NULL, NULL, "-f", filename, sizeof(filename), &flg));
  if (!flg) {
    if (!rank) {
      PetscPrintf(PETSC_COMM_SELF,
                  "Usage: %s -f <matrix_binary_file>\n", argv[0]);
    }
    PetscCall(PetscFinalize());
    return 1;
  }

  /* Load matrix A from PETSc binary */
  {
    PetscViewer viewer;
    PetscCall(PetscViewerBinaryOpen(PETSC_COMM_WORLD, filename, FILE_MODE_READ, &viewer));
    PetscCall(MatCreate(PETSC_COMM_WORLD, &A));
    PetscCall(MatSetFromOptions(A));
    PetscCall(MatLoad(A, viewer));
    PetscCall(PetscViewerDestroy(&viewer));
  }
  if (!rank) {
    PetscPrintf(PETSC_COMM_SELF, "Loaded: %s\n", filename);
  }

  PetscCall(MatGetSize(A, &M, &N));

  /* Create compatible vectors */
  PetscCall(VecCreate(PETSC_COMM_WORLD, &x));
  PetscCall(VecSetSizes(x, PETSC_DECIDE, N));
  PetscCall(VecSetFromOptions(x));
  PetscCall(VecDuplicate(x, &y));

  srand(42);

  PetscCall(VecGetOwnershipRange(x, &Istart, &Iend));
  for (i = Istart; i < Iend; ++i) {
    double random_decimal = (double)rand() / (double)RAND_MAX;

    PetscScalar val = (PetscScalar)(random_decimal);
    PetscCall(VecSetValue(x, i, val, INSERT_VALUES));
  }
  PetscCall(VecAssemblyBegin(x));
  PetscCall(VecAssemblyEnd(x));

  /* ------------------------------
     TIMED SPMV: y = A * x
     ------------------------------ */

  MPI_Barrier(PETSC_COMM_WORLD);

  if (!rank) PetscPrintf(PETSC_COMM_SELF, "Warm up: \n");

  // warmup
  for (int i = 0; i < NUM_RUNS; i++) {
    PetscCall(MatMult(A, x, y));
    MPI_Barrier(PETSC_COMM_WORLD);
  }

  if (!rank) PetscPrintf(PETSC_COMM_SELF, "Experiment: \n");

  PetscLogStage stage;

  double total = 0;

  for (int i = 0; i < NUM_RUNS; i++) {
    char buf[50];
    snprintf(buf, sizeof(buf), "SpMV %d", i);
    PetscLogStageRegister(buf, &stage);
    PetscLogStagePush(stage);

    double t0 = MPI_Wtime();
    PetscCall(MatMult(A, x, y));
    PetscLogStagePop();
    MPI_Barrier(PETSC_COMM_WORLD);
    double t1 = MPI_Wtime();

    double local_s = (t1 - t0);
    double max_s;
    MPI_Reduce(&local_s, &max_s, 1, MPI_DOUBLE, MPI_MAX, 0, PETSC_COMM_WORLD);
    total += max_s;
    if (!rank) {
        PetscPrintf(PETSC_COMM_SELF,
                  "Run %d: %f\n",
                  i, max_s);
    }
  }

  if (!rank) {
      PetscPrintf(PETSC_COMM_SELF,
                  "Average: %f\n", total / NUM_RUNS);
    }

  /* Cleanup */
  PetscCall(VecDestroy(&x));
  PetscCall(VecDestroy(&y));
  PetscCall(MatDestroy(&A));
  PetscCall(PetscFinalize());
  return 0;
}