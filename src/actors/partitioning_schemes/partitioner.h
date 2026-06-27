#ifndef PARTITIONER_H
#define PARTITIONER_H

struct Partitioner {
    virtual inline int64_t matrix_get_owner(int64_t row, int64_t col) = 0;
    virtual inline int64_t matrix_row_to_local_row(int64_t row) = 0;
    virtual inline int64_t matrix_col_to_local_col(int64_t col) = 0;
    virtual inline int64_t vector_get_owner(int64_t row) = 0;
    virtual inline int64_t vector_row_to_local_row(int64_t row) = 0;
    virtual void allocate_vectors(int64_t m, int64_t n, int64_t& local_rows, int64_t& local_cols, int64_t& local_vector_size, double*& vec, double*& out) = 0;
};

#endif // PARTITIONER_H