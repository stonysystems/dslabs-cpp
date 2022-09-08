#pragma once

#include "../__dep__.h"
#include "../procedure.h"
#include "tapir_rpc.h"
#include "scheduler.h"

namespace janus {

class TapirServiceImpl : public TapirService {
  SchedulerTapir* dtxn_sched_ = nullptr;
  void TapirAccept(const txid_t& cmd_id,
                   const ballot_t& ballot,
                   const int32_t& decision,
                   rrr::DeferredReply* defer) override;
  void TapirFastAccept(const txid_t& cmd_id,
                       const vector<SimpleCommand>& txn_cmds,
                       rrr::i32* res,
                       rrr::DeferredReply* defer) override;
  void TapirDecide(const txid_t& cmd_id,
                   const rrr::i32& decision,
                   rrr::DeferredReply* defer) override;



};

} // namespace janus
