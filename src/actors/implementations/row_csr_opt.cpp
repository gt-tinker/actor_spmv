#include "../util.h"
#include "../matrix_formats/csr.h"
#include <unordered_map>

typedef struct PullOPTPkt {
    double val;
    int64_t col;
} PullOPTPkt;

class PullOPTSelector: public hclib::Selector<2, PullOPTPkt> {
public:
    PullOPTSelector(Problem* problem, 
        std::unordered_map<int64_t, double> &cache) : 
        hclib::Selector<2, PullOPTPkt>(true),
        problem_(problem), cache_(cache) {
        mb[0].process = [this] (PullOPTPkt pkt, int sender_rank) { 
            this->req_process(pkt, sender_rank);
        };
        mb[1].process = [this] (PullOPTPkt pkt, int sender_rank) { 
            this->resp_process(pkt, sender_rank);
        };
    }

private:
    Problem* problem_;
    std::unordered_map<int64_t, double> &cache_;

    void req_process(PullOPTPkt pkg, int sender_rank) {
        int64_t lrow = problem_->partitioner->vector_row_to_local_row(pkg.col);
        pkg.val = problem_->vec[lrow];
        send(1, pkg, sender_rank);
    }

    void resp_process(PullOPTPkt pkg, int sender_rank) {
        cache_.find(pkg.col)->second = pkg.val; 
    }
};

double row_csr_opt(Problem* problem, CSR* mtx, int run_number) 
{
    uint64_t comm_vol = 0;
    std::unordered_map<int64_t, double> cache;
    PullOPTSelector* spSelector = new PullOPTSelector(problem, cache);
    double t1 = wall_seconds();
    hclib::finish([&]() 
    {
        //comm_vol = 0;

        PullOPTPkt pkg;
        for (int64_t i = 0; i < mtx->local_rows; i++) {
            for(int64_t j = mtx->rowptr[i]; j < mtx->rowptr[i+1]; j++) {
                double val = mtx->vals[j];
                int64_t col = mtx->colind[j];
                int64_t owner = problem->partitioner->vector_get_owner(col);
                if (owner != MYTHREAD) {
                    if(cache.find(col) == cache.end()) {
                        cache.insert(std::make_pair(col, -1));
                        pkg.col = col;
                        spSelector->send(0, pkg, owner);
                        //comm_vol += 1;
                    }
                } 
            }
        }
        spSelector->done(0);
    });
    // uint64_t total_comm = lgp_reduce_add_l(comm_vol);
    // T0_fprintf(stderr, "Vol: %ld bytes\n", total_comm*sizeof(PullOPTPkt));

    for (int64_t i = 0; i < mtx->local_rows; i++) {
        for(int64_t j = mtx->rowptr[i]; j < mtx->rowptr[i+1]; j++) {
            double val = mtx->vals[j];
            int64_t col = mtx->colind[j];
            int64_t owner = problem->partitioner->vector_get_owner(col);
            if (owner == MYTHREAD) {
                problem->out[i] += val * problem->vec[problem->partitioner->vector_row_to_local_row(col)];
            } else {
                problem->out[i] += val * cache.find(col)->second;
            }
        }
    }
    t1 = wall_seconds() - t1;
    delete spSelector;
    return t1;
}