#ifndef DCSR_H
#define DCSR_H

struct DCSR {
    std::vector<int64_t> rowind;   // indices of non-empty rows
    std::vector<int64_t> rowptr;   // size = rowind.size() + 1
    std::vector<int64_t> colind;
    std::vector<double> vals;

    DCSR(const std::vector<Coordinate>& coo)
    {
        // Reserve space (avoids reallocations)
        colind.reserve(coo.size());
        vals.reserve(coo.size());

        // 2. Build structure
        uint64_t current_row = coo[0].row;

        rowind.push_back(current_row);
        rowptr.push_back(0);

        for (int64_t i = 0; i < coo.size(); ++i) {
            if (coo[i].row != current_row) {
                current_row = coo[i].row;
                rowind.push_back(current_row);
                rowptr.push_back(i);
            }

            colind.push_back(coo[i].col);
            vals.push_back(coo[i].val);
        }

        // 3. Final pointer
        rowptr.push_back(coo.size());
    }
};

#endif // DCSR_H