#include "../__dep__.h"
#include "../service.h"
#include "../marshallable.h"
#include "../classic/tx.h"
#include "scheduler.h"
#include "rpc_service.h"

namespace janus {
void CarouselServiceImpl::CarouselReadAndPrepare(const i64& cmd_id,
    const MarshallDeputy& md, const bool_t& leader, int32_t* res, TxnOutput* output,
    rrr::DeferredReply* defer) {
  // TODO: yidawu
  shared_ptr<Marshallable> sp = md.sp_data_;
  *res = SUCCESS;
  auto sched = (SchedulerCarousel*)svr_;
	DepId di;
	di.str = "dep";
	di.id = 0;
  if (!sched->Dispatch(cmd_id, di, sp, *output)) {
    *res = REJECT;
  }
  if (*res == SUCCESS) {
    std::vector<i32> sids;
    if (leader) {
      *res = sched->OnPrepare(cmd_id) ? SUCCESS : REJECT;
    } else {
      // Followers try to do prepare directly.
      bool ret = sched->DoPrepare(cmd_id);
      if (!ret) {
        const auto& func = [res, defer, cmd_id, sids, sched, this]() {
          auto sp_tx = dynamic_pointer_cast<TxClassic>(sched->GetOrCreateTx(cmd_id));
          sp_tx->prepare_result->Wait();
          bool ret2 = sp_tx->prepare_result->Get();
          *res = ret2 ? SUCCESS : REJECT;
          defer->reply();
        };
        Coroutine::CreateRun(func);
        return;
      }
    }
  }
  defer->reply();
}

void CarouselServiceImpl::CarouselAccept(const cmdid_t& cmd_id, const ballot_t& ballot,
    const int32_t& decision, rrr::DeferredReply* defer) {
  verify(0);
}

void CarouselServiceImpl::CarouselFastAccept(const txid_t& tx_id,
    const vector<SimpleCommand>& txn_cmds, rrr::i32* res, rrr::DeferredReply* defer) {
  /*SchedulerCarousel* sched = (SchedulerCarousel*) dtxn_sched_;
  *res = sched->OnFastAccept(tx_id, txn_cmds);
  defer->reply();*/
}

void CarouselServiceImpl::CarouselDecide(
    const cmdid_t& cmd_id, const rrr::i32& decision, rrr::DeferredReply* defer) {
  SchedulerCarousel* sched = (SchedulerCarousel*)svr_;
  sched->OnDecide(cmd_id, decision, [defer]() { defer->reply(); });
}
}
