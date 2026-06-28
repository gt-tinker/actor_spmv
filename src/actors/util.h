#ifndef UTIL_H
#define UTIL_H

#include "shmem.h"
#include <math.h>
extern "C" {
#include "spmat.h"
}
#include <std_options.h>
#include <iostream>
#include <fstream>
// #define ENABLE_TCOMM_PROFILING
#include "selector.h"
#include <vector>
#include <algorithm>
#include <limits>
#include <iostream>
#include <memory>
#include <random>
#include "partitioning_schemes/partitioner.h"
#include "partitioning_schemes/column_cyclic.h"
#include "partitioning_schemes/column_block.h"
#include "partitioning_schemes/row_cyclic.h"
#include "partitioning_schemes/row_block.h"
#include <cstdlib>

#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()
#define NUM_RUNS 10

struct Coordinate {
    int64_t row;
    int64_t col;
    double val;
};

struct Configuration {
    enum class Partition { BLOCK, CYCLIC };
    enum class Dimension { ROW, COLUMN };
    enum class Format { CSR, CSC };

    Partition partition;
    Dimension dimension;
    Format format;

    bool verify = false;
};

struct Problem {
    std::vector<Coordinate> coo;
    int64_t rows;
    int64_t cols;
    int64_t local_rows;
    int64_t local_cols;
    int64_t local_vector_size;

    Partitioner* partitioner;

    double* vec;
    double* out;
    std::vector<double> expected_output;
};

static bool nearly_equal(double a, double b) {
    static const double rel_tol = 1e-9;
    static const double abs_tol = 1e-12;
    return fabs(a - b) <= fmax(rel_tol * fmax(fabs(a), fabs(b)), abs_tol);
}

// Matches C-style random number generation used in petsc baseline
inline double random_double() {
    // only seeded once
    static bool seeded = [] {
        srand(42);
        return true;
    }();

    (void)seeded; // avoids unused variable warning
    return static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
}

Problem* read_matrix_market(const std::string& filename, Configuration config);

#endif