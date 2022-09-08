#include "__dep__.h"
#include "benchmark_control_rpc.h"
#include "classic/scheduler.h"
#include "classic/tpc_command.h"
#include "classic/tx.h"
#include "command.h"
#include "command_marshaler.h"
#include "communicator.h"
#include "config.h"
#include "coordinator.h"
#include "procedure.h"
#include "service.h"
#include "scheduler.h"

namespace janus {

ClassicServiceImpl::ClassicServiceImpl(TxLogServer* sched,
                                       rrr::PollMgr* poll_mgr,
                                       ServerControlServiceImpl* scsi) : scsi_(
    scsi), dtxn_sched_(sched), poll_mgr_(poll_mgr) {

#ifdef PIECE_COUNT
  piece_count_timer_.start();
  piece_count_prepare_fail_ = 0;
  piece_count_prepare_success_ = 0;
#endif

  if (Config::GetConfig()->do_logging()) {
    verify(0); // TODO disable logging for now.
    auto path = Config::GetConfig()->log_path();
//    recorder_ = new Recorder(path);
//    poll_mgr->add(recorder_);
  }

  this->RegisterStats();
}

void ClassicServiceImpl::ReElect(bool_t* success,
																 rrr::DeferredReply* defer) {
	for(int i = 0; i < 100000; i++) Log_info("loop loop loop");
	*success = dtxn_sched()->RequestVote();
	defer->reply();
}
void ClassicServiceImpl::Dispatch(const i64& cmd_id,
																	const DepId& dep_id,
                                  const MarshallDeputy& md,
                                  int32_t* res,
                                  TxnOutput* output,
                                  uint64_t* coro_id,
                                  rrr::DeferredReply* defer) {
#ifdef PIECE_COUNT
  piece_count_key_t piece_count_key =
      (piece_count_key_t){header.t_type, header.p_type};
  std::map<piece_count_key_t, uint64_t>::iterator pc_it =
      piece_count_.find(piece_count_key);

  if (pc_it == piece_count_.end())
      piece_count_[piece_count_key] = 1;
  else
      piece_count_[piece_count_key]++;
  piece_count_tid_.insert(header.tid);
#endif
  shared_ptr<Marshallable> sp = md.sp_data_;
	//Log_info("CreateRunning2");
  // Coroutine::CreateRun([cmd_id, sp, output, res, coro_id, this, defer]() {
    *res = SUCCESS;
    if (!dtxn_sched()->Dispatch(cmd_id, sp, *output)) {
      *res = REJECT;
    }
    *coro_id = Coroutine::CurrentCoroutine()->id;
    defer->reply();
  // }, __FILE__, cmd_id);
  // auto func = [cmd_id, sp, output, dep_id, res, coro_id, this, defer]() {
  //   *res = SUCCESS;
  //   auto sched = (SchedulerClassic*) dtxn_sched_;
  //   if (!sched->Dispatch(cmd_id, dep_id, sp, *output)) {
  //     *res = REJECT;
  //   }
  //   *coro_id = Coroutine::CurrentCoroutine()->id;
  //   defer->reply();
  // };

  // auto sched = (SchedulerClassic*) dtxn_sched_;
  // auto tx = dynamic_pointer_cast<TxClassic>(sched->GetOrCreateTx(cmd_id));
	// func();
}


void ClassicServiceImpl::FailOverTrig(
    const bool_t& pause, rrr::i32* res, rrr::DeferredReply* defer) {
  if (pause && clt_cnt_ == 0) {
    // TODO temp solution yidawu
    auto client_infos = Config::GetConfig()->GetMyClients();
    unordered_set<string> clt_set;
    for (auto c : client_infos) {
      clt_set.insert(c.host);
    }

    clt_cnt_.store(clt_set.size());
  }

  Coroutine::CreateRun([&]() {
    if (pause) {
      // TODO: yidawu need to test with multi clients in diff machines
      int wait_int = 50 * 1000; // 50ms
      while (clt_cnt_.load() == 0) {
        auto e = Reactor::CreateSpEvent<NeverEvent>();
        e->Wait(wait_int);
      }
      clt_cnt_--;
      while (clt_cnt_.load() != 0) {
        auto e = Reactor::CreateSpEvent<NeverEvent>();
        e->Wait(wait_int);
      }
      dtxn_sched_->rep_sched_->Pause();
      poll_mgr_->pause();
    } else {
      poll_mgr_->resume();
      dtxn_sched_->rep_sched_->Resume();
    }
    *res = SUCCESS;
    defer->reply();
  });
}

void ClassicServiceImpl::SimpleCmd(
    const SimpleCommand& cmd, rrr::i32* res, rrr::DeferredReply* defer) {
  Coroutine::CreateRun([res, defer, this]() {
    auto empty_cmd = std::make_shared<TpcEmptyCommand>();
    verify(empty_cmd->kind_ == MarshallDeputy::CMD_TPC_EMPTY);
    auto sp_m = dynamic_pointer_cast<Marshallable>(empty_cmd);
    auto sched = (SchedulerClassic*)dtxn_sched_;
    sched->CreateRepCoord(0)->Submit(sp_m);
    empty_cmd->Wait();
    *res = SUCCESS;
    defer->reply();
  });
}

void ClassicServiceImpl::IsLeader(
    const locid_t& can_id, bool_t* is_leader, rrr::DeferredReply* defer) {
  auto sched = (SchedulerClassic*)dtxn_sched_;
  *is_leader = sched->IsLeader();
  defer->reply();
}

void ClassicServiceImpl::IsFPGALeader(
    const locid_t& can_id, bool_t* is_leader, rrr::DeferredReply* defer) {
  auto sched = (SchedulerClassic*)dtxn_sched_;
  *is_leader = sched->IsFPGALeader();
  defer->reply();
}
void ClassicServiceImpl::Prepare(const rrr::i64& tid,
                                 const std::vector<i32>& sids,
                                 const DepId& dep_id,
                                 rrr::i32* res,
																 bool_t* slow,
                                 uint64_t* coro_id,
                                 rrr::DeferredReply* defer) {
//  std::lock_guard<std::mutex> guard(mtx_);
  const auto& func = [res, slow, coro_id, defer, tid, sids, dep_id, this]() {
    auto sched = (SchedulerClassic*) dtxn_sched_;
		bool null_cmd = false;
    bool ret = sched->OnPrepare(tid, sids, dep_id, null_cmd);
		//Log_info("slow1: %d", sched->slow_);
		*slow = sched->slow_;
    *res = ret ? SUCCESS : REJECT;
		if(null_cmd) *res = REPEAT;
    *coro_id = Coroutine::CurrentCoroutine()->id;
    if (defer != nullptr) defer->reply();
  };

	//Log_info("CreateRunning2");
	func();
  //auto coro = Coroutine::CreateRun(func);
  //Log_info("coro id on service side: %d", coro->id);
// TODO move the stat to somewhere else.
#ifdef PIECE_COUNT
  std::map<piece_count_key_t, uint64_t>::iterator pc_it;
  if (*res != SUCCESS)
      piece_count_prepare_fail_++;
  else
      piece_count_prepare_success_++;
  if (piece_count_timer_.elapsed() >= 5.0) {
      Log::info("PIECE_COUNT: txn served: %u", piece_count_tid_.size());
      Log::info("PIECE_COUNT: prepare success: %llu, failed: %llu",
        piece_count_prepare_success_, piece_count_prepare_fail_);
      for (pc_it = piece_count_.begin(); pc_it != piece_count_.end(); pc_it++)
          Log::info("PIECE_COUNT: t_type: %d, p_type: %d, started: %llu",
            pc_it->first.t_type, pc_it->first.p_type, pc_it->second);
      piece_count_timer_.start();
  }
#endif
}

void ClassicServiceImpl::Commit(const rrr::i64& tid,
                                const DepId& dep_id,
                                rrr::i32* res,
																bool_t* slow,
                                uint64_t* coro_id,
																Profiling* profile,
                                rrr::DeferredReply* defer) {
  //std::lock_guard<std::mutex> guard(mtx_);
  const auto& func = [tid, res, slow, coro_id, dep_id, profile, defer, this]() {
    auto sched = (SchedulerClassic*) dtxn_sched_;
    sched->OnCommit(tid, dep_id, SUCCESS);
    std::vector<double> result = rrr::CPUInfo::cpu_stat();
    *profile = {result[0], result[1], result[2], result[3]};
		//*profile = {0.0, 0.0, 0.0, 0.0};
		//Log_info("slow2: %d", sched->slow_);
		*slow = sched->slow_;
    *res = SUCCESS;
    *coro_id = Coroutine::CurrentCoroutine()->id;
    defer->reply();
  };
	//Log_info("CreateRunning2");
  //Coroutine::CreateRun(func);
	func();
}

void ClassicServiceImpl::Abort(const rrr::i64& tid,
                               const DepId& dep_id,
                               rrr::i32* res,
															 bool_t* slow,
                               uint64_t* coro_id,
															 Profiling* profile,
                               rrr::DeferredReply* defer) {

  Log_debug("get abort_txn: tid: %ld", tid);
  //std::lock_guard<std::mutex> guard(mtx_);
  const auto& func = [tid, res, slow, coro_id, dep_id, profile, defer, this]() {
    auto sched = (SchedulerClassic*) dtxn_sched_;
    sched->OnCommit(tid, dep_id, REJECT);
    std::vector<double> result = rrr::CPUInfo::cpu_stat();
    *profile = {result[0], result[1], result[2]};
		Log_info("slow3: %d", sched->slow_);
		*slow = sched->slow_;
    *res = SUCCESS;
    *coro_id = Coroutine::CurrentCoroutine()->id;
    defer->reply();
  };
	//Log_info("CreateRunning2");
  //Coroutine::CreateRun(func);
	func();
}

void ClassicServiceImpl::EarlyAbort(const rrr::i64& tid,
                                    rrr::i32* res,
                                    rrr::DeferredReply* defer) {
  Log_debug("get abort_txn: tid: %ld", tid);
//  std::lock_guard<std::mutex> guard(mtx_);
//  const auto& func = [tid, res, defer, this]() {
  auto sched = (SchedulerClassic*) dtxn_sched_;
  sched->OnEarlyAbort(tid);
  *res = SUCCESS;
  defer->reply();
//  };
//  Coroutine::CreateRun(func);
}

void ClassicServiceImpl::rpc_null(rrr::DeferredReply* defer) {
  defer->reply();
}

void ClassicServiceImpl::UpgradeEpoch(const uint32_t& curr_epoch,
                                      int32_t* res,
                                      DeferredReply* defer) {
  *res = dtxn_sched()->OnUpgradeEpoch(curr_epoch);
  defer->reply();
}

void ClassicServiceImpl::TruncateEpoch(const uint32_t& old_epoch,
                                       DeferredReply* defer) {
  verify(0);
}



void ClassicServiceImpl::RegisterStats() {
  if (scsi_) {
    scsi_->set_recorder(recorder_);
    scsi_->set_recorder(recorder_);
    scsi_->set_stat(ServerControlServiceImpl::STAT_SZ_SCC, &stat_sz_scc_);
    scsi_->set_stat(ServerControlServiceImpl::STAT_SZ_GRAPH_START,
                    &stat_sz_gra_start_);
    scsi_->set_stat(ServerControlServiceImpl::STAT_SZ_GRAPH_COMMIT,
                    &stat_sz_gra_commit_);
    scsi_->set_stat(ServerControlServiceImpl::STAT_SZ_GRAPH_ASK,
                    &stat_sz_gra_ask_);
    scsi_->set_stat(ServerControlServiceImpl::STAT_N_ASK, &stat_n_ask_);
    scsi_->set_stat(ServerControlServiceImpl::STAT_RO6_SZ_VECTOR,
                    &stat_ro6_sz_vector_);
  }
}

void ClassicServiceImpl::MsgString(const string& arg,
                                   string* ret,
                                   rrr::DeferredReply* defer) {

  verify(comm_ != nullptr);
  for (auto& f : comm_->msg_string_handlers_) {
    if (f(arg, *ret)) {
      defer->reply();
      return;
    }
  }
  verify(0);
  return;
}

void ClassicServiceImpl::MsgMarshall(const MarshallDeputy& arg,
                                     MarshallDeputy* ret,
                                     rrr::DeferredReply* defer) {

  verify(comm_ != nullptr);
  for (auto& f : comm_->msg_marshall_handlers_) {
    if (f(arg, *ret)) {
      defer->reply();
      return;
    }
  }
  verify(0);
  return;
}

} // namespace janus
