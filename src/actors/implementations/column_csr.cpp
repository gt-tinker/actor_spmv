#include "../util.h"
#include "../matrix_formats/csr.h"
#include "selectors/push_selector.h"

double column_csr(Problem* problem, CSR* mtx, int run_number) 
{
    double t1 = wall_seconds();

    PushSelector* spSelector = new PushSelector(problem);
    hclib::finish([=]() 
    {
        spSelector->start();

        PushPkt pkg;
        for (int64_t i = 0; i < mtx->local_rows; i++) {
            if (mtx->rowptr[i] == mtx->rowptr[i+1]) continue;

            double sum = 0;
            for(int64_t j = mtx->rowptr[i]; j < mtx->rowptr[i+1]; j++) {
                double matching_vec_val = problem->vec[mtx->colind[j]];
                sum += mtx->vals[j] * matching_vec_val;
            }
            pkg.psum = sum;
            pkg.row = i;
            int64_t owner = problem->partitioner->vector_get_owner(i);
            if (problem->partitioner->vector_get_owner(i) == MYTHREAD) {
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