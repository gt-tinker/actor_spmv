#ifndef COLUMN_CYCLIC_H
#define COLUMN_CYCLIC_H

#include "partitioner.h"

struct ColumnCyclic : public Partitioner {
    int64_t matrix_get_owner(int64_t row, int64_t col) override {
        return col % THREADS;
    }

    int64_t matrix_row_to_local_row(int64_t row) override {
        return row;
    }

    int64_t matrix_col_to_local_col(int64_t col) override {
        return col / THREADS;
    }

    int64_t vector_get_owner(int64_t row) override {
        return row % THREADS;
    }

    int64_t vector_row_to_local_row(int64_t row) override {
        return row / THREADS;
    }

    void allocate_vectors(int64_t m, int64_t n, int64_t& local_rows, int64_t& local_cols, int64_t& local_vector_size, double*& vec, double*& out) override {
        local_cols = n / THREADS;
        int64_t remainder = n % THREADS;
        if (MYTHREAD < remainder) {
            local_cols++;
        }
        local_rows = m;
        local_vector_size = local_cols;
        
        vec = (double*) lgp_all_alloc(m, sizeof(double));
        out = (double*) lgp_all_alloc(m, sizeof(double));
        if (!vec || !out) lgp_global_exit(1);
    }
};

#endif // COLUMN_CYCLIC_H