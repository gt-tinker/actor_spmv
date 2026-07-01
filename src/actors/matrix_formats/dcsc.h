#ifndef DCSC_H
#define DCSC_H

struct DCSC {
    std::vector<int64_t> colind;   // indices of non-empty columns
    std::vector<int64_t> colptr;   // size = colind.size() + 1
    std::vector<int64_t> rowind;
    std::vector<double> vals;

    DCSC(const std::vector<Coordinate>& coo)
    {
        if (coo.empty())
            return;

        // Reserve space (avoids reallocations)
        rowind.reserve(coo.size());
        vals.reserve(coo.size());

        // Build structure
        int64_t current_col = coo[0].col;

        colind.push_back(current_col);
        colptr.push_back(0);

        for (int64_t i = 0; i < coo.size(); ++i) {
            if (coo[i].col != current_col) {
                current_col = coo[i].col;
                colind.push_back(current_col);
                colptr.push_back(i);
            }

            rowind.push_back(coo[i].row);
            vals.push_back(coo[i].val);
        }

        // Final pointer
        colptr.push_back(coo.size());
    }
};

#endif // DCSC_H