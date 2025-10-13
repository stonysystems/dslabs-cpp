#include "rpc_service.h"
#include "server.h"
#include "../janus/scheduler.h"

namespace janus {

void RococoServiceImpl::RccDispatch(const vector<SimpleCommand>& cmd,
                                     int32_t* res,
                                     TxnOutput* output,
                                     MarshallDeputy* p_md_graph,
                                     DeferredReply* defer) {
//  std::lock_guard<std::mutex> guard(this->mtx_);
  RccServer* sched = (RccServer*) svr_;
  p_md_graph->SetMarshallable(std::make_shared<RccGraph>());
  auto p = dynamic_pointer_cast<RccGraph>(p_md_graph->sp_data_);
  *res = sched->OnDispatch(cmd, output, p);
  defer->reply();
}

void RococoServiceImpl::RccFinish(const cmdid_t& cmd_id,
                                   const MarshallDeputy& md_graph,
                                   TxnOutput* output,
                                   DeferredReply* defer) {
  const RccGraph& graph = dynamic_cast<const RccGraph&>(*md_graph.sp_data_);
  verify(graph.size() > 0);
  verify(0);
//  std::lock_guard<std::mutex> guard(mtx_);
  RccServer* sched = (RccServer*) svr_;
//  sched->OnCommit(cmd_id, RANK_UNDEFINED, graph, output, [defer]() { defer->reply(); });

//  stat_sz_gra_commit_.sample(graph.size());
}

void RococoServiceImpl::RccInquire(const txnid_t& tid,
                                    const int32_t& rank,
                                    map<txid_t, parent_set_t>* ret,
                                    rrr::DeferredReply* defer) {
//  verify(IS_MODE_RCC || IS_MODE_RO6);
//  std::lock_guard<std::mutex> guard(mtx_);
  RccServer* p_sched = (RccServer*) svr_;
//  p_md_graph->SetMarshallable(std::make_shared<RccGraph>());
//  p_sched->OnInquire(epoch,
//                     tid,
//                     dynamic_pointer_cast<RccGraph>(p_md_graph->sp_data_));
  p_sched->OnInquire(tid, rank, ret);
  defer->reply();
}


void RococoServiceImpl::RccDispatchRo(const SimpleCommand& cmd,
                                       map<int32_t, Value>* output,
                                       rrr::DeferredReply* defer) {
//  std::lock_guard<std::mutex> guard(mtx_);
  verify(0);
  auto tx = svr_->GetOrCreateTx(cmd.root_id_, true);
  auto dtxn = dynamic_pointer_cast<RccTx>(tx);
  dtxn->start_ro(cmd, *output, defer);
}

void RococoServiceImpl::RccInquireValidation(
    const txid_t& txid,
    const int32_t& rank,
    int32_t* ret,
    DeferredReply* defer) {
  auto* s = (RccServer*) svr_;
  *ret = s->OnInquireValidation(txid, rank);
  defer->reply();
}

void RococoServiceImpl::RccNotifyGlobalValidation(
    const txid_t& txid, const int32_t& rank, const int32_t& res, DeferredReply* defer) {
  auto* s = (RccServer*) svr_;
  s->OnNotifyGlobalValidation(txid, rank, res);
  defer->reply();
}

void RococoServiceImpl::JanusDispatch(const vector<SimpleCommand>& cmd,
                                       int32_t* p_res,
                                       TxnOutput* p_output,
                                       MarshallDeputy* p_md_res_graph,
                                       DeferredReply* p_defer) {
//    std::lock_guard<std::mutex> guard(this->mtx_); // TODO remove the lock.
  auto sp_graph = std::make_shared<RccGraph>();
  auto* sched = (SchedulerJanus*) svr_;
  *p_res = sched->OnDispatch(cmd, p_output, sp_graph);
  if (sp_graph->size() <= 1) {
    p_md_res_graph->SetMarshallable(std::make_shared<EmptyGraph>());
  } else {
    p_md_res_graph->SetMarshallable(sp_graph);
  }
  verify(p_md_res_graph->kind_ != MarshallDeputy::UNKNOWN);
  p_defer->reply();
}

void RococoServiceImpl::JanusCommit(const cmdid_t& cmd_id,
                                     const rank_t& rank,
                                     const int32_t& need_validation,
                                     const MarshallDeputy& graph,
                                     int32_t* res,
                                     TxnOutput* output,
                                     DeferredReply* defer) {
//  std::lock_guard<std::mutex> guard(mtx_);
  verify(0);
  auto sp_graph = dynamic_pointer_cast<RccGraph>(graph.sp_data_);
  auto p_sched = (RccServer*) svr_;
  *res = p_sched->OnCommit(cmd_id, rank, need_validation, sp_graph, output);
  defer->reply();
}

void RococoServiceImpl::RccCommit(const cmdid_t& cmd_id,
                                   const rank_t& rank,
                                   const int32_t& need_validation,
                                   const parent_set_t& parents,
                                   int32_t* res,
                                   TxnOutput* output,
                                   DeferredReply* defer) {
//  std::lock_guard<std::mutex> guard(mtx_);
  auto p_sched = (RccServer*) svr_;
  *res = p_sched->OnCommit(cmd_id, rank, need_validation, parents, output);
  defer->reply();
}

void RococoServiceImpl::JanusCommitWoGraph(const cmdid_t& cmd_id,
                                            const rank_t& rank,
                                            const int32_t& need_validation,
                                            int32_t* res,
                                            TxnOutput* output,
                                            DeferredReply* defer) {
//  std::lock_guard<std::mutex> guard(mtx_);
  verify(0);
  auto sched = (SchedulerJanus*) svr_;
  *res = sched->OnCommit(cmd_id, rank, need_validation, nullptr, output);
  defer->reply();
}

void RococoServiceImpl::JanusInquire(const epoch_t& epoch,
                                      const cmdid_t& tid,
                                      MarshallDeputy* p_md_graph,
                                      rrr::DeferredReply* defer) {
  verify(0);
//  std::lock_guard<std::mutex> guard(mtx_);
//  p_md_graph->SetMarshallable(std::make_shared<RccGraph>());
//  auto p_sched = (SchedulerJanus*) svr_;
//  p_sched->OnInquire(epoch,
//                     tid,
//                     dynamic_pointer_cast<RccGraph>(p_md_graph->sp_data_));
//  defer->reply();
}

void RococoServiceImpl::RccPreAccept(const cmdid_t& txnid,
                                      const rank_t& rank,
                                      const vector<SimpleCommand>& cmds,
                                      int32_t* res,
                                      parent_set_t* res_parents,
                                      DeferredReply* defer) {
//  std::lock_guard<std::mutex> guard(mtx_);
  auto sched = (RccServer*) svr_;
  *res = sched->OnPreAccept(txnid, rank, cmds, *res_parents);
  defer->reply();
}

void RococoServiceImpl::JanusPreAccept(const cmdid_t& txnid,
                                        const rank_t& rank,
                                        const vector<SimpleCommand>& cmds,
                                        const MarshallDeputy& md_graph,
                                        int32_t* res,
                                        MarshallDeputy* p_md_res_graph,
                                        DeferredReply* defer) {
//  std::lock_guard<std::mutex> guard(mtx_);
  p_md_res_graph->SetMarshallable(std::make_shared<RccGraph>());
  auto sp_graph = dynamic_pointer_cast<RccGraph>(md_graph.sp_data_);
  auto ret_sp_graph = dynamic_pointer_cast<RccGraph>(p_md_res_graph->sp_data_);
  verify(sp_graph);
  verify(ret_sp_graph);
  auto sched = (SchedulerJanus*) svr_;
  *res = sched->OnPreAccept(txnid, rank, cmds, sp_graph, ret_sp_graph);
  defer->reply();
}

void RococoServiceImpl::JanusPreAcceptWoGraph(const cmdid_t& txnid,
                                               const rank_t& rank,
                                               const vector<SimpleCommand>& cmds,
                                               int32_t* res,
                                               MarshallDeputy* res_graph,
                                               DeferredReply* defer) {
//  std::lock_guard<std::mutex> guard(mtx_);
  res_graph->SetMarshallable(std::make_shared<RccGraph>());
  auto* p_sched = (SchedulerJanus*) svr_;
  auto sp_ret_graph = dynamic_pointer_cast<RccGraph>(res_graph->sp_data_);
  *res = p_sched->OnPreAccept(txnid, rank, cmds, nullptr, sp_ret_graph);
  defer->reply();
}

void RococoServiceImpl::RccAccept(const cmdid_t& txnid,
                                   const rank_t& rank,
                                   const ballot_t& ballot,
                                   const parent_set_t& parents,
                                   int32_t* res,
                                   DeferredReply* defer) {
  auto sched = (RccServer*) svr_;
  *res = sched->OnAccept(txnid, rank, ballot, parents);
  defer->reply();
}

void RococoServiceImpl::JanusAccept(const cmdid_t& txnid,
                                     const int32_t& rank,
                                     const ballot_t& ballot,
                                     const MarshallDeputy& md_graph,
                                     int32_t* res,
                                     DeferredReply* defer) {
  auto graph = dynamic_pointer_cast<RccGraph>(md_graph.sp_data_);
  verify(graph);
  verify(md_graph.kind_ == MarshallDeputy::RCC_GRAPH);
  auto sched = (SchedulerJanus*) svr_;
  sched->OnAccept(txnid, rank, ballot, graph, res);
  defer->reply();
}

void RococoServiceImpl::PreAcceptFebruus(const txid_t& tx_id,
                                          int32_t* res,
                                          uint64_t* timestamp,
                                          DeferredReply* defer) {
//  SchedulerFebruus* sched = (SchedulerFebruus*) svr_;
//  *res = sched->OnPreAccept(tx_id, *timestamp);
//  defer->reply();
}

void RococoServiceImpl::AcceptFebruus(const txid_t& tx_id,
                                       const ballot_t& ballot,
                                       const uint64_t& timestamp,
                                       int32_t* res,
                                       DeferredReply* defer) {
//  SchedulerFebruus* sched = (SchedulerFebruus*) svr_;
//  *res = sched->OnAccept(tx_id, timestamp, ballot);
//  defer->reply();
//
}

void RococoServiceImpl::CommitFebruus(const txid_t& tx_id,
                                       const uint64_t& timestamp,
                                       int32_t* res,
                                       DeferredReply* defer) {
//  SchedulerFebruus* sched = (SchedulerFebruus*) svr_;
//  *res = sched->OnCommit(tx_id, timestamp);
//  defer->reply();
}
}

