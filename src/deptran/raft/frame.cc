#include "../__dep__.h"
#include "../constants.h"
#include "frame.h"
#include "exec.h"
#include "coordinator.h"
#include "server.h"
#include "service.h"
#include "commo.h"
#include "config.h"
#include "test.h"

namespace janus {

REG_FRAME(MODE_FPGA_RAFT, vector<string>({"raft"}), RaftFrame);

/*
template<typename D>
struct automatic_register {
 private:
  struct exec_register {
    exec_register() {
      D::do_it();
    }
  };
  // will force instantiation of definition of static member
  template<exec_register&> struct ref_it { };

  static exec_register register_object;
  static ref_it<register_object> referrer;
};

template<typename D> typename automatic_register<D>::exec_register
    automatic_register<D>::register_object;

struct foo : automatic_register<foo> {
  static void do_it() {
    REG_FRAME(MODE_FPGA_RAFT, vector<string>({"fpga_raft"}), RaftFrame);
  }
};*/

RaftFrame::RaftFrame(int mode) : Frame(mode) {

}

#ifdef RAFT_TEST_CORO
std::mutex RaftFrame::raft_test_mutex_;
std::shared_ptr<Coroutine> RaftFrame::raft_test_coro_ = nullptr;
uint16_t RaftFrame::n_replicas_ = 0;
RaftFrame *RaftFrame::replicas_[5];
uint16_t RaftFrame::n_commo_ = 0;
bool RaftFrame::tests_done_ = false;
#endif

Executor *RaftFrame::CreateExecutor(cmdid_t cmd_id, TxLogServer *sched) {
  Executor *exec = new RaftExecutor(cmd_id, sched);
  return exec;
}

Coordinator *RaftFrame::CreateCoordinator(cooid_t coo_id,
                                                Config *config,
                                                int benchmark,
                                                ClientControlServiceImpl *ccsi,
                                                uint32_t id,
                                                shared_ptr<TxnRegistry> txn_reg) {
  verify(config != nullptr);
  CoordinatorRaft *coo;
  coo = new CoordinatorRaft(coo_id,
                                  benchmark,
                                  ccsi,
                                  id);
  coo->frame_ = this;
  verify(commo_ != nullptr);
  coo->commo_ = commo_;
  /* TODO: remove when have a class for common data */
  verify(svr_ != nullptr);
  coo->svr_ = this->svr_;
  coo->slot_hint_ = &slot_hint_;
  coo->slot_id_ = slot_hint_++;
  coo->n_replica_ = config->GetPartitionSize(site_info_->partition_id_);
  coo->loc_id_ = this->site_info_->locale_id;
  verify(coo->n_replica_ != 0); // TODO
  Log_debug("create new fpga raft coord, coo_id: %d", (int) coo->coo_id_);
  return coo;
}

TxLogServer *RaftFrame::CreateScheduler() {
  if(svr_ == nullptr)
  {
    svr_ = new RaftServer(this);
  }
  else
  {
    verify(0) ;
  }
  Log_debug("create new fpga raft sched loc: %d", this->site_info_->locale_id);

#ifdef RAFT_TEST_CORO
  raft_test_mutex_.lock();
  verify(n_replicas_ < 5);
  replicas_[n_replicas_++] = this;
  raft_test_mutex_.unlock();
#endif

  return svr_ ;
}

Communicator *RaftFrame::CreateCommo(PollMgr *poll) {
  // We only have 1 instance of RaftFrame object that is returned from
  // GetFrame method. RaftCommo currently seems ok to share among the
  // clients of this method.
  if (commo_ == nullptr) {
    commo_ = new RaftCommo(poll);
  }

  #ifdef RAFT_TEST_CORO
  raft_test_mutex_.lock();
  verify(n_replicas_ == 5);
  for (int i = 0; i < 5; i++) {
    if (replicas_[i] == this) {
      verify(n_commo_ < 5);
      n_commo_++;
      break;
    }
  }
  raft_test_mutex_.unlock();

  if (site_info_->locale_id == 0) {
    verify(raft_test_coro_.get() == nullptr);
    Log_debug("Creating Raft test coroutine");
    raft_test_coro_ = Coroutine::CreateRun([this] () {
      // Yield until all 5 communicators are initialized
      Coroutine::CurrentCoroutine()->Yield();
      // Run tests
      verify(n_replicas_ == 5);
      auto testconfig = new RaftTestConfig(replicas_);
      RaftLabTest test(testconfig);
      test.Run();
      test.Cleanup();
      // Turn off Reactor loop
      Reactor::GetReactor()->looping_ = false;
      return;
    });
    Log_info("raft_test_coro_ id=%d", raft_test_coro_->id);
    // wait until n_commo_ == 5, then resume the coroutine
    raft_test_mutex_.lock();
    while (n_commo_ < 5) {
      raft_test_mutex_.unlock();
      sleep(0.1);
      raft_test_mutex_.lock();
    }
    raft_test_mutex_.unlock();
    Reactor::GetReactor()->ContinueCoro(raft_test_coro_);
  }
  #endif

  return commo_;
}

vector<rrr::Service *>
RaftFrame::CreateRpcServices(uint32_t site_id,
                                   TxLogServer *rep_sched,
                                   rrr::PollMgr *poll_mgr,
                                   ServerControlServiceImpl *scsi) {
  auto config = Config::GetConfig();
  auto result = std::vector<Service *>();
  switch (config->replica_proto_) {
    case MODE_FPGA_RAFT:result.push_back(new RaftServiceImpl(rep_sched));
    default:break;
  }
  return result;
}

} // namespace janus;
