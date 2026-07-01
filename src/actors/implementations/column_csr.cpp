#include "../util.h"
#include "./selectors/push_selector.h"
#include "../matrix_formats/dcsr.h"

double column_csr(Problem* problem, DCSR* mtx, int run_number) 
{
    double t1 = wall_seconds();

    PushSelector* spSelector = new PushSelector(problem);
    hclib::finish([=]() 
    {
        spSelector->start();

        PushPkt pkg;

        const int64_t num_rows = mtx->rowind.size(); // number of non-empty rows
        // stagger start position
        int64_t r = MYTHREAD * (num_rows / THREADS);
        // int64_t r = 0;

        for (int64_t _ = 0; _ < num_rows; ++_, r++) {
            if (r >= num_rows) {
                r = 0;
            }
            int64_t i = mtx->rowind[r];  // original row index

            double sum = 0.0;
            for (int64_t k = mtx->rowptr[r]; k < mtx->rowptr[r + 1]; ++k) {
                sum += mtx->vals[k] * problem->vec[mtx->colind[k]];
            }

            int64_t owner = problem->partitioner->vector_get_owner(i);
            pkg.psum = sum;
            pkg.row = i;
            if (owner == MYTHREAD) {
                problem->out[problem->partitioner->vector_row_to_local_row(i)] += pkg.psum;
            } else {
                spSelector->send(REQUEST, pkg, owner);
            }
        }
        spSelector->done(REQUEST);
    });
    lgp_barrier();
    t1 = wall_seconds() - t1;

    #ifdef ENABLE_TCOMM_PROFILING
    if (run_number >= 0) {
        std::string label = "Run " + std::to_string(run_number);
        spSelector->print_profiling(label.data());
        lgp_barrier();
    }
    #endif

    delete spSelector;
    return t1;
}