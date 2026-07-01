#ifndef CSR_H
#define CSR_H

struct CSR {
    std::vector<int64_t> rowptr;
    std::vector<int64_t> colind;
    std::vector<double> vals;

    CSR(std::vector<Coordinate>& coo) {
        rowptr.assign(coo.size() + 1, 0);

        for (int64_t p = 0; p < coo.size(); ++p)
            rowptr[coo[p].row + 1]++;

        for (int64_t r = 0; r < rowptr.size() - 1; ++r)
            rowptr[r + 1] += rowptr[r];

        colind.resize(coo.size());
        vals.resize(coo.size());

        std::vector<int64_t> write_ptr = rowptr;

        for (long long p = 0; p < coo.size(); ++p) {
            int row = coo[p].row;
            int dst = write_ptr[row]++;

            colind[dst] = coo[p].col;
            vals[dst]   = coo[p].val;
        }
    }
};

#endif // CSR_H