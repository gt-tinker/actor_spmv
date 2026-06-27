#ifndef COLUMN_BLOCK_H
#define COLUMN_BLOCK_H

#include "partitioner.h"

struct ColumnBlock : public Partitioner {
    int64_t cols_per_pe;
    int64_t col_start;
    int64_t local_cols;

    ColumnBlock(int64_t cols_per_pe, int64_t col_start, int64_t local_cols) : cols_per_pe(cols_per_pe), col_start(col_start), local_cols(local_cols) {}

    int64_t matrix_get_owner(int64_t row, int64_t col) override {
        return col / cols_per_pe;
    }

    int64_t matrix_row_to_local_row(int64_t row) override {
        return row;
    }

    int64_t matrix_col_to_local_col(int64_t col) override {
        return col % cols_per_pe;
    }

    int64_t vector_get_owner(int64_t row) override {
        return row / cols_per_pe;
    }

    int64_t vector_row_to_local_row(int64_t row) override {
        return row % cols_per_pe;
    }

    void allocate_vectors(int64_t m, int64_t n, int64_t& local_rows, int64_t& local_cols, int64_t& local_vector_size, double*& vec, double*& out) override {
        local_cols = this->local_cols;
        local_rows = m;
        local_vector_size = local_cols;

        vec = (double*) shmem_malloc(cols_per_pe * sizeof(double));
        out = (double*) shmem_malloc(cols_per_pe * sizeof(double));

        if (!vec || !out)
            shmem_global_exit(1);

        shmem_barrier_all();
        lgp_barrier();
    }
};

#endif // COLUMN_BLOCK_H