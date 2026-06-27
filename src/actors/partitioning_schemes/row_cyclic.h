#ifndef ROW_CYCLIC_H
#define ROW_CYCLIC_H

#include "partitioner.h"

struct RowCyclic : public Partitioner {
    int64_t matrix_get_owner(int64_t row, int64_t col) override {
        return row % THREADS;
    }

    int64_t matrix_row_to_local_row(int64_t row) override {
        return row / THREADS;
    }

    int64_t matrix_col_to_local_col(int64_t col) override {
        return col;
    }

    int64_t vector_get_owner(int64_t row) override {
        return row % THREADS;
    }

    int64_t vector_row_to_local_row(int64_t row) override {
        return row / THREADS;
    }

    void allocate_vectors(int64_t m, int64_t n, int64_t& local_rows, int64_t& local_cols, int64_t& local_vector_size, double*& vec, double*& out) override {
        local_rows = m / THREADS;
        int64_t remainder = m % THREADS;
        if (MYTHREAD < remainder) {
            local_rows++;
        }
        local_cols = n;
        local_vector_size = local_rows;
        
        vec = (double*) lgp_all_alloc(m, sizeof(double));
        out = (double*) lgp_all_alloc(m, sizeof(double));
        if (!vec || !out) lgp_global_exit(1);
    }
};

#endif // ROW_CYCLIC_H