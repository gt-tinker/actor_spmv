#include "../util.h"
#include "../matrix_formats/dcsc.h"

typedef struct SpmvPkt {
    double val;
    int64_t col_ind;
    int64_t col;
} SpmvPkt;

enum MailBoxType {REQUEST, RESPONSE};

class SpmvSelector: public hclib::Selector<2, SpmvPkt> {
public:
    SpmvSelector(Problem* problem, DCSC* mtx) : problem_(problem), mtx_(mtx) {
        mb[REQUEST].process = [this] (SpmvPkt pkt, int sender_rank) { 
            this->req_process(pkt, sender_rank);
        };
        mb[RESPONSE].process = [this] (SpmvPkt pkt, int sender_rank) { 
            this->resp_process(pkt, sender_rank);
        };
    }

private:
    Problem* problem_;
    DCSC* mtx_;

    void req_process(SpmvPkt pkg, int sender_rank) {
        int64_t lrow = problem_->partitioner->vector_row_to_local_row(pkg.col);
        pkg.val = problem_->vec[lrow];

        send(RESPONSE, pkg, sender_rank);
    }

    void resp_process(SpmvPkt pkg, int sender_rank) {
        for(int64_t j = mtx_->colptr[pkg.col_ind]; j < mtx_->colptr[pkg.col_ind+1]; j++) {
            double val = mtx_->vals[j];
            int64_t row = mtx_->rowind[j];
            problem_->out[row] += val * pkg.val;
        }
    }
};

double row_csc(Problem* problem, DCSC* mtx, int run_number) 
{
    double t1 = wall_seconds();

    SpmvSelector* spSelector = new SpmvSelector(problem, mtx);
    hclib::finish([=]() 
    {
        spSelector->start();

        SpmvPkt pkg;
        const int64_t num_cols = mtx->colind.size(); // number of non-empty columns
        // stagger start position
        int64_t c = MYTHREAD * (num_cols / THREADS);

        for (int64_t _ = 0; _ < num_cols; ++_, ++c) {
            if (c >= num_cols) {
                c = 0;
            }
            int64_t col = mtx->colind[c]; // original column

            int64_t owner = problem->partitioner->vector_get_owner(col);
            if (owner == MYTHREAD) {
                double vec_val = problem->vec[problem->partitioner->vector_row_to_local_row(col)];
                for (int64_t j = mtx->colptr[c]; j < mtx->colptr[c+1]; j++) {
                    double val = mtx->vals[j];
                    int64_t row = mtx->rowind[j];
                    problem->out[row] += val * vec_val;
                }
            } else {
                pkg.col = col;
                pkg.col_ind = c;
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