#ifndef ROW_BLOCK_H
#define ROW_BLOCK_H

#include "partitioner.h"

struct RowBlock : public Partitioner {
    int64_t rows_per_pe;
    int64_t row_start;
    int64_t local_rows;

    RowBlock(int64_t rows_per_pe, int64_t row_start, int64_t local_rows) : rows_per_pe(rows_per_pe), row_start(row_start), local_rows(local_rows) {}

    int64_t matrix_get_owner(int64_t row, int64_t col) override {
        return row / rows_per_pe;
    }

    int64_t matrix_row_to_local_row(int64_t row) override {
        return row % rows_per_pe;
    }

    int64_t matrix_col_to_local_col(int64_t col) override {
        return col;
    }

    int64_t vector_get_owner(int64_t row) override {
        return row / rows_per_pe;
    }

    int64_t vector_row_to_local_row(int64_t row) override {
        return row % rows_per_pe;
    }

    void allocate_vectors(int64_t m, int64_t n, int64_t& local_rows, int64_t& local_cols, int64_t& local_vector_size, double*& vec, double*& out) override {
        local_rows = this->local_rows;
        local_cols = n;
        local_vector_size = local_rows;

        vec = (double*) shmem_malloc(rows_per_pe * sizeof(double));
        out = (double*) shmem_malloc(rows_per_pe * sizeof(double));

        if (!vec || !out)
            shmem_global_exit(1);

        shmem_barrier_all();
        lgp_barrier();
    }
};

#endif // ROW_BLOCK_H