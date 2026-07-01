#include "../util.h"
#include "../matrix_formats/csc.h"
#include "selectors/push_selector.h"

double column_csc(Problem* problem, CSC* mtx, int run_number) 
{
    double t1 = wall_seconds();

    PushSelector* spSelector = new PushSelector(problem);
    hclib::finish([=]() 
    {
        spSelector->start();

        PushPkt pkg;
        for (int64_t i = 0; i < problem->local_cols; i++) {
            double matching_vec_val = problem->vec[i];
            
            for(int64_t j = mtx->colptr[i]; j < mtx->colptr[i+1]; j++) {
                double val = mtx->vals[j];
                int64_t row = mtx->rowind[j];
                int64_t owner = problem->partitioner->vector_get_owner(row);
                pkg.psum = val * matching_vec_val;
                pkg.row = row;
                if (problem->partitioner->vector_get_owner(row) == MYTHREAD) {
                    problem->out[problem->partitioner->vector_row_to_local_row(row)] += pkg.psum;
                } else {
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