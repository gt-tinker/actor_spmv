#ifndef PUSH_SELECTOR
#define PUSH_SELECTOR

#include "../../util.h"

typedef struct PushPkt {
    double psum;
    int64_t row;
} PushPkt;

enum MailBoxType {REQUEST};

class PushSelector: public hclib::Selector<1, PushPkt> {
public:
    PushSelector(Problem* problem) : problem_(problem) {
        mb[REQUEST].process = [this] (PushPkt pkt, int sender_rank) { 
            this->req_process(pkt, sender_rank);
        };
    }

private:
    Problem* problem_;

    void req_process(PushPkt pkg, int sender_rank) {
        int64_t lrow = problem_->partitioner->vector_row_to_local_row(pkg.row);
        problem_->out[lrow] += pkg.psum;
    }
};
#endif // PUSH_SELECTOR