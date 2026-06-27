#ifndef CSC_H
#define CSC_H

struct CSC {
    int64_t global_rows;      // global row count
    int64_t local_rows;
    int64_t global_cols;      // local column count
    int64_t local_cols;

    std::vector<int64_t> colptr;
    std::vector<int64_t> rowind;
    std::vector<double> vals;

    CSC(int64_t m, int64_t n, int64_t local_cols, std::vector<Coordinate>& coo) {
        global_rows = m;
        global_cols = n;
        this->local_cols = local_cols;
        local_rows = m;
        colptr.assign(local_cols + 1, 0);

        for (int64_t p = 0; p < coo.size(); ++p)
            colptr[coo[p].col + 1]++;

        for (int64_t r = 0; r < colptr.size() - 1; ++r)
            colptr[r + 1] += colptr[r];

        rowind.resize(coo.size());
        vals.resize(coo.size());

        std::vector<int64_t> write_ptr = colptr;

        for (long long p = 0; p < coo.size(); ++p) {
            int col = coo[p].col;
            int dst = write_ptr[col]++;

            rowind[dst] = coo[p].row;
            vals[dst]   = coo[p].val;
        }
    }
};
#endif // CSC_H