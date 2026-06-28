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

#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()
#define NUM_RUNS 1

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

inline std::mt19937 rng(42); // seed (use fixed for reproducibility, or random_device)
inline std::uniform_real_distribution<double> dist(0.0, 1.0);
inline double random_double() {
    return dist(rng);
}

Problem* read_matrix_market(const std::string& filename, Configuration config);

#endif