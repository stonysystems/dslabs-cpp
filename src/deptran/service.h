#pragma once

#include "__dep__.h"
#include "procedure.h"
#include "marshallable.h"
#include "rcc_rpc.h"

#define DepTranServiceImpl ClassicServiceImpl

namespace janus {

class ServerControlServiceImpl;
class TxLogServer;
class SimpleCommand;
class Communicator;
class SchedulerClassic;
//class rrr::PollMgr ;

class ClassicServiceImpl : public ClassicService {

 public:
  AvgStat stat_sz_gra_start_;
  AvgStat stat_sz_gra_commit_;
  AvgStat stat_sz_gra_ask_;
  AvgStat stat_sz_scc_;
  AvgStat stat_n_ask_;
  AvgStat stat_ro6_sz_vector_;
  uint64_t n_asking_ = 0;

//  std::mutex mtx_;
  Recorder* recorder_{nullptr};
  ServerControlServiceImpl* scsi_; // for statistics;
  Communicator* comm_{nullptr};

  TxLogServer* dtxn_sched_;
  rrr::PollMgr* poll_mgr_;
  std::atomic<int32_t> clt_cnt_{0};

  TxLogServer* dtxn_sched() {
    return dtxn_sched_;
  }

  void rpc_null(DeferredReply* defer) override ;

	void ReElect(bool_t* success,
							 DeferredReply* defer) override;

  void Dispatch(const i64& cmd_id,
								const DepId& dep_id,
                const MarshallDeputy& cmd,
                int32_t* res,
                TxnOutput* output,
                uint64_t* coro_id,
                DeferredReply* defer_reply) override;


  void FailOverTrig(const bool_t& pause, 
                        rrr::i32* res, 
                        rrr::DeferredReply* defer) override ;

  void IsLeader(const locid_t& can_id,
                 bool_t* is_leader,
                 DeferredReply* defer_reply) override ;

  void IsFPGALeader(const locid_t& can_id,
                 bool_t* is_leader,
                 DeferredReply* defer_reply) override ;

  void SimpleCmd (const SimpleCommand& cmd, 
                      i32* res, DeferredReply* defer_reply) override ;

  void Prepare(const i64& tid,
               const std::vector<i32>& sids,
               const DepId& dep_id,
               i32* res,
							 bool_t* slow,
               uint64_t* coro_id,
               DeferredReply* defer) override;

  void Commit(const i64& tid,
              const DepId& dep_id,
              i32* res,
							bool_t* slow,
              uint64_t* coro_id,
	        		Profiling* profile,
              DeferredReply* defer) override;

  void Abort(const i64& tid,
             const DepId& dep_id,
             i32* res,
						 bool_t* slow,
             uint64_t* coro_id,
	        	 Profiling* profile,
             DeferredReply* defer) override;

  void EarlyAbort(const i64& tid,
                  i32* res,
                  DeferredReply* defer) override;

  void UpgradeEpoch(const uint32_t& curr_epoch,
                    int32_t* res,
                    DeferredReply* defer) override;

  void TruncateEpoch(const uint32_t& old_epoch,
                     DeferredReply* defer) override;

  void MsgString(const string& arg,
                 string* ret,
                 rrr::DeferredReply* defer) override;

  void MsgMarshall(const MarshallDeputy& arg,
                   MarshallDeputy* ret,
                   rrr::DeferredReply* defer) override;

#ifdef PIECE_COUNT
  typedef struct piece_count_key_t{
      i32 t_type;
      i32 p_type;
      bool operator<(const piece_count_key_t &rhs) const {
          if (t_type < rhs.t_type)
              return true;
          else if (t_type == rhs.t_type && p_type < rhs.p_type)
              return true;
          return false;
      }
  } piece_count_key_t;

  std::map<piece_count_key_t, uint64_t> piece_count_;

  std::unordered_set<i64> piece_count_tid_;

  uint64_t piece_count_prepare_fail_, piece_count_prepare_success_;

  base::Timer piece_count_timer_;
#endif

 public:

  ClassicServiceImpl() = delete;

  ClassicServiceImpl(TxLogServer* sched,
                     rrr::PollMgr* poll_mgr,
                     ServerControlServiceImpl* scsi = NULL);

 protected:
  void RegisterStats();
};

} // namespace janus

