#include "../util.h"
#include "../matrix_formats/csr.h"
#include "selectors/pull_selector.h"

double row_csr(Problem* problem, CSR* mtx, int run_number) 
{
    double t1 = wall_seconds();

    PullSelector* spSelector = new PullSelector(problem);
    hclib::finish([=]() 
    {
        spSelector->start();

        PullPkt pkg;
        for (int64_t i = 0; i < mtx->local_rows; i++) {
            for(int64_t j = mtx->rowptr[i]; j < mtx->rowptr[i+1]; j++) {
                double val = mtx->vals[j];
                int64_t col = mtx->colind[j];
                int64_t owner = problem->partitioner->vector_get_owner(col);
                if (owner == MYTHREAD) {
                    problem->out[i] += val * problem->vec[problem->partitioner->vector_row_to_local_row(col)];
                } else {
                    pkg.psum = val;
                    pkg.lrow = i;
                    pkg.col = col;
                    spSelector->send(REQUEST, pkg, owner);
                }
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