#pragma once
#include "../scheduler.h"
#include "carousel_rpc.h"

namespace janus {
class CarouselServiceImpl: public CarouselService {
  TxLogServer* svr_;
  void CarouselReadAndPrepare(const i64& cmd_id, const MarshallDeputy& cmd,
      const bool_t& leader, int32_t* res, TxnOutput* output,
      DeferredReply* defer_reply) override;
  void CarouselAccept(const txid_t& cmd_id, const ballot_t& ballot,
      const int32_t& decision, rrr::DeferredReply* defer) override;
  void CarouselFastAccept(const txid_t& cmd_id, const vector<SimpleCommand>& txn_cmds,
      rrr::i32* res, rrr::DeferredReply* defer) override;
  void CarouselDecide(
      const txid_t& cmd_id, const rrr::i32& decision, rrr::DeferredReply* defer) override;

};
}
