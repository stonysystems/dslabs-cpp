
#include "../marshallable.h"
#include "service.h"
#include "server.h"

namespace janus {

RaftServiceImpl::RaftServiceImpl(TxLogServer *sched)
    : svr_((RaftServer*)sched) {
	struct timespec curr_time;
	clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
	srand(curr_time.tv_nsec);
}


void RaftServiceImpl::HandleRequestVote(const uint64_t& arg1,
                                        const uint64_t& arg2,
                                        uint64_t *ret1,
                                        bool_t *vote_granted,
                                        rrr::DeferredReply* defer) {
  /* Your code here */
  *ret1 = 0;
  *vote_granted = false;
  defer->reply();
}

void RaftServiceImpl::HandleAppendEntries(const MarshallDeputy& md_cmd,
                                          bool_t *followerAppendOK,
                                          rrr::DeferredReply* defer) {
  /* Your code here */
  std::shared_ptr<Marshallable> cmd = const_cast<MarshallDeputy&>(md_cmd).sp_data_;
  *followerAppendOK = false;
  defer->reply();
}

void RaftServiceImpl::HandleHelloRpc(const string& req,
                                     string* res,
                                     rrr::DeferredReply* defer) {
  /* Your code here */
  Log_info("receive an rpc: %s", req.c_str());
  *res = "world";
  defer->reply();
}

} // namespace janus;
