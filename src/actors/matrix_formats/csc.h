#ifndef CSC_H
#define CSC_H

struct CSC {
    std::vector<int64_t> colptr;
    std::vector<int64_t> rowind;
    std::vector<double> vals;

    CSC(std::vector<Coordinate>& coo) {
        colptr.assign(coo.size() + 1, 0);

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