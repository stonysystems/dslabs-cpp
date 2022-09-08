
#include "rpc_service.h"
#include "scheduler.h"

namespace janus {

void TapirServiceImpl::TapirAccept(const cmdid_t& cmd_id,
                                     const ballot_t& ballot,
                                     const int32_t& decision,
                                     rrr::DeferredReply* defer) {
  verify(0);
}

void TapirServiceImpl::TapirFastAccept(const txid_t& tx_id,
                                         const vector<SimpleCommand>& txn_cmds,
                                         rrr::i32* res,
                                         rrr::DeferredReply* defer) {
  SchedulerTapir* sched = (SchedulerTapir*) dtxn_sched_;
  *res = sched->OnFastAccept(tx_id, txn_cmds);
  defer->reply();
}

void TapirServiceImpl::TapirDecide(const cmdid_t& cmd_id,
                                     const rrr::i32& decision,
                                     rrr::DeferredReply* defer) {
  SchedulerTapir* sched = (SchedulerTapir*) dtxn_sched_;
  sched->OnDecide(cmd_id, decision, [defer]() { defer->reply(); });
}

} // namespace janus
