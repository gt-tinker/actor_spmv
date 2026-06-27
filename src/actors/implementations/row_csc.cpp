#include "../util.h"
#include "../matrix_formats/csc.h"

typedef struct SpmvPkt {
    double val;
    int64_t col;
} SpmvPkt;

enum MailBoxType {REQUEST, RESPONSE};

class SpmvSelector: public hclib::Selector<2, SpmvPkt> {
public:
    SpmvSelector(Problem* problem, CSC* mtx) : problem_(problem), mtx_(mtx) {
        mb[REQUEST].process = [this] (SpmvPkt pkt, int sender_rank) { 
            this->req_process(pkt, sender_rank);
        };
        mb[RESPONSE].process = [this] (SpmvPkt pkt, int sender_rank) { 
            this->resp_process(pkt, sender_rank);
        };
    }

private:
    Problem* problem_;
    CSC* mtx_;

    void req_process(SpmvPkt pkg, int sender_rank) {
        int64_t lrow = problem_->partitioner->vector_row_to_local_row(pkg.col);
        pkg.val = problem_->vec[lrow];

        send(RESPONSE, pkg, sender_rank);
    }

    void resp_process(SpmvPkt pkg, int sender_rank) {
        for(int64_t j = mtx_->colptr[pkg.col]; j < mtx_->colptr[pkg.col+1]; j++) {
            double val = mtx_->vals[j];
            int64_t row = mtx_->rowind[j];
            problem_->out[row] += val * pkg.val;
        }
    }
};

double row_csc(Problem* problem, CSC* mtx, int run_number) 
{
    double t1 = wall_seconds();

    SpmvSelector* spSelector = new SpmvSelector(problem, mtx);
    hclib::finish([=]() 
    {
        spSelector->start();

        SpmvPkt pkg;
        for (int64_t i = 0; i < mtx->local_cols; i++) {
            if (mtx->colptr[i] == mtx->colptr[i+1]) continue;

            pkg.col = i;
            int64_t owner = problem->partitioner->vector_get_owner(i);
            spSelector->send(REQUEST, pkg, owner);
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