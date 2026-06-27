#ifndef PULL_SELECTOR_H
#define PULL_SELECTOR_H

#include "../../util.h"

typedef struct PullPkt {
    double psum;
    int64_t lrow;
    int64_t col;
} PullPkt;

enum MailBoxType {REQUEST, RESPONSE};

class PullSelector: public hclib::Selector<2, PullPkt> {
public:
    PullSelector(Problem* problem) : problem_(problem) {
        mb[REQUEST].process = [this] (PullPkt pkt, int sender_rank) { 
            this->req_process(pkt, sender_rank);
        };
        mb[RESPONSE].process = [this] (PullPkt pkt, int sender_rank) { 
            this->resp_process(pkt, sender_rank);
        };
    }

private:
    Problem* problem_;

    void req_process(PullPkt pkg, int sender_rank) {
        int64_t lrow = problem_->partitioner->vector_row_to_local_row(pkg.col);
        pkg.psum *= problem_->vec[lrow];

        send(RESPONSE, pkg, sender_rank);
    }

    void resp_process(PullPkt pkg, int sender_rank) {
        problem_->out[pkg.lrow] += pkg.psum;
    }
};



#endif // PULL_SELECTOR_H