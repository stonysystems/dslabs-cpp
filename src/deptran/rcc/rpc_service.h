#pragma once

#include "tx.h"
#include "../procedure.h"
#include "../marshallable.h"
#include "rococo_rpc.h"
#include "server.h"

namespace janus {

class RococoServiceImpl : public RococoService {

  RccServer* svr_;

  void RccDispatch(const vector<SimpleCommand>& cmd,
                   int32_t* res,
                   TxnOutput* output,
                   MarshallDeputy* p_md_graph,
                   DeferredReply* defer) override;

  void RccPreAccept(const txid_t& txnid,
                    const rank_t& rank,
                    const vector<SimpleCommand>& cmd,
                    int32_t* res,
                    parent_set_t* parents,
                    DeferredReply* defer) override;


  void RccAccept(const txid_t& txnid,
                 const rank_t& rank,
                 const ballot_t& ballot,
                 const parent_set_t& parents,
                 int32_t* res,
                 DeferredReply* defer) override;

  void RccCommit(const txid_t& cmd_id,
                 const rank_t& rank,
                 const int32_t& need_validation,
                 const parent_set_t& parents,
                 int32_t* res,
                 TxnOutput* output,
                 DeferredReply* defer) override;

  void RccFinish(const txid_t& cmd_id,
                 const MarshallDeputy& md_graph,
                 TxnOutput* output,
                 DeferredReply* defer) override;

  void RccInquire(const txid_t& tid,
                  const int32_t& rank,
                  map<txid_t, parent_set_t>*,
                  DeferredReply*) override;

  void RccDispatchRo(const SimpleCommand& cmd,
                     map<int32_t, Value>* output,
                     DeferredReply* reply) override;

  void RccInquireValidation(const txid_t& txid, const int32_t& rank, int32_t* ret, DeferredReply* reply) override;
  void RccNotifyGlobalValidation(const txid_t& txid, const int32_t& rank, const int32_t& res, DeferredReply* reply) override;

  void JanusDispatch(const vector<SimpleCommand>& cmd,
                     int32_t* p_res,
                     TxnOutput* p_output,
                     MarshallDeputy* p_md_res_graph,
                     DeferredReply* p_defer) override;

  void JanusCommit(const txid_t& cmd_id,
                   const rank_t& rank,
                   const int32_t& need_validation,
                   const MarshallDeputy& graph,
                   int32_t* res,
                   TxnOutput* output,
                   DeferredReply* defer) override;

  void JanusCommitWoGraph(const txid_t& cmd_id,
                          const rank_t& rank,
                          const int32_t& need_validation,
                          int32_t* res,
                          TxnOutput* output,
                          DeferredReply* defer) override;

  void JanusInquire(const epoch_t& epoch,
                    const txid_t& tid,
                    MarshallDeputy* p_md_graph,
                    DeferredReply*) override;

  void JanusPreAccept(const txid_t& txnid,
                      const rank_t& rank,
                      const vector<SimpleCommand>& cmd,
                      const MarshallDeputy& md_graph,
                      int32_t* res,
                      MarshallDeputy* p_md_res_graph,
                      DeferredReply* defer) override;

  void JanusPreAcceptWoGraph(const txid_t& txnid,
                             const rank_t& rank,
                             const vector<SimpleCommand>& cmd,
                             int32_t* res,
                             MarshallDeputy* res_graph,
                             DeferredReply* defer) override;

  void JanusAccept(const txid_t& txnid,
                   const rank_t& rank,
                   const ballot_t& ballot,
                   const MarshallDeputy& md_graph,
                   int32_t* res,
                   DeferredReply* defer) override;

  void PreAcceptFebruus(const txid_t& tx_id,
                        int32_t* res,
                        uint64_t* timestamp,
                        DeferredReply* defer) override;

  void AcceptFebruus(const txid_t& tx_id,
                     const ballot_t& ballot,
                     const uint64_t& timestamp,
                     int32_t* res,
                     DeferredReply* defer) override;

  void CommitFebruus(const txid_t& tx_id,
                     const uint64_t& timestamp,
                     int32_t* res, DeferredReply* defer) override;

};

}

