#include "server_worker.h"
#include "service.h"
#include "benchmark_control_rpc.h"
#include "sharding.h"
#include "scheduler.h"
#include "frame.h"
#include "communicator.h"
#include "../kv/server.h"
#include "../kv/service.h"
#include "../shardkv/server.h"
#include "../shardkv/service.h"
#include "../shardmaster/service.h"
#ifdef RAFT_TEST_CORO
#include "raft/frame.h"
#include "../../test/labtest.h"
#include "../../test/labtestconf.h"
#endif

namespace janus {

void ServerWorker::SetupHeartbeat() {
  bool hb = Config::GetConfig()->do_heart_beat();
  if (!hb) return;
  auto timeout = Config::GetConfig()->get_ctrl_timeout();
  scsi_ = new ServerControlServiceImpl(timeout);
  int n_io_threads = 1;
//  svr_hb_poll_thread_worker_g = new rrr::PollThreadWorker(n_io_threads);
  svr_hb_poll_thread_worker_g = svr_poll_thread_worker_;
//  hb_thread_pool_g = new rrr::ThreadPool(1);
  hb_thread_pool_g = svr_thread_pool_;
  hb_rpc_server_ = new rrr::Server(svr_hb_poll_thread_worker_g, hb_thread_pool_g);
  hb_rpc_server_->reg(scsi_);

  auto port = this->site_info_->port + ServerWorker::CtrlPortDelta;
  std::string addr_port = std::string("0.0.0.0:") +
      std::to_string(port);
  hb_rpc_server_->start(addr_port.c_str());
  if (hb_rpc_server_ != nullptr) {
    // Log_info("notify ready to control script for %s", bind_addr.c_str());
    scsi_->set_ready();
  }
  Log_info("heartbeat setup for %s on %s",
           this->site_info_->name.c_str(), addr_port.c_str());
}

void ServerWorker::SetupBase() {
  auto config = Config::GetConfig();
  tx_frame_ = Frame::GetFrame(config->tx_proto_);
  tx_frame_->site_info_ = site_info_;

  // this needs to be done before poping table
  sharding_ = tx_frame_->CreateSharding(Config::GetConfig()->sharding_);
  sharding_->BuildTableInfoPtr();

  verify(tx_reg_ == nullptr);
  tx_reg_ = std::make_shared<TxnRegistry>();
  tx_sched_ = tx_frame_->CreateScheduler();
  tx_sched_->txn_reg_ = tx_reg_;
  tx_sched_->SetPartitionId(site_info_->partition_id_);
  tx_sched_->loc_id_ = site_info_->locale_id;
  tx_sched_->site_id_ = site_info_->id;
  sharding_->tx_sched_ = tx_sched_;

  Log_info("Is it replicated: %d", config->IsReplicated());
  if (config->IsReplicated() &&
      config->replica_proto_ != config->tx_proto_) {
    rep_frame_ = Frame::GetFrame(config->replica_proto_);
    rep_frame_->site_info_ = site_info_;
    rep_sched_ = rep_frame_->CreateScheduler();
    rep_sched_->txn_reg_ = tx_reg_;
    rep_sched_->loc_id_ = site_info_->locale_id;
    rep_sched_->site_id_ = site_info_->id;
    rep_sched_->partition_id_ = site_info_->partition_id_;
    rep_sched_->tx_sched_ = tx_sched_;
    tx_sched_->rep_frame_ = rep_frame_;
    tx_sched_->rep_sched_ = rep_sched_;
    rep_log_svr_.reset(rep_sched_);
  }
  // add callbacks to execute commands to rep_sched_
  if (rep_sched_ && tx_sched_) {
    rep_sched_->RegLearnerAction(
        std::bind(static_cast<int(TxLogServer::*)(int, shared_ptr<Marshallable>)>(&TxLogServer::Next),
                  tx_sched_,
                  std::placeholders::_1,
                  std::placeholders::_2));
  }

  // Setup KV and Shard lab servers
  auto& yaml = Config::GetConfig()->yaml_config_;
  if (yaml["lab"].IsDefined() && yaml["lab"]["shard"].IsDefined() &&
      yaml["lab"]["shard"].as<bool>()) {
    if (this->site_info_->partition_id_ == 0) {
      sm_svr_ = make_shared<ShardMasterServiceImpl>();
      sm_svr_->sp_log_svr_ = rep_log_svr_;
      verify(sm_svr_->sp_log_svr_);
      rep_log_svr_->app_next_ = [this](int slot, shared_ptr<Marshallable> m) -> int {
        if (m) sm_svr_->OnNextCommand(*m);
        return 0;
      };
    } else {
      auto sk_svr = make_shared<ShardKvServer>();
      shardkv_svr_ = sk_svr;
      shardkv_svr_->sp_log_svr_ = rep_log_svr_;
      verify(shardkv_svr_->sp_log_svr_);
      rep_log_svr_->app_next_ = [sk_svr](int slot, shared_ptr<Marshallable> m) -> int {
        if (m) sk_svr->OnNextCommand(*m);
        return 0;
      };
    }
  } else if (yaml["lab"].IsDefined() && yaml["lab"]["kv"].IsDefined() &&
             yaml["lab"]["kv"].as<bool>()) {
    auto kv_svr = make_shared<KvServer>();
    kv_svr_ = kv_svr;
    kv_svr_->sp_log_svr_ = rep_log_svr_;
    verify(kv_svr_->sp_log_svr_);
    rep_log_svr_->app_next_ = [kv_svr](int slot, shared_ptr<Marshallable> m) -> int {
      if (m) kv_svr->OnNextCommand(*m);
      return 0;
    };
  }
}

void ServerWorker::PopTable() {
  // Skip table population if no benchmark configured (e.g., for lab tests)
  if (sharding_->tb_infos_.size() == 0) {
    verify(!Config::GetConfig()->benchmark());
    return;
  }

  // populate table
  int ret = 0;
  // get all tables
  std::vector<std::string> table_names;

  Log_info("start data population for site %d", site_info_->id);
  ret = sharding_->GetTableNames(site_info_->partition_id_, table_names);
  verify(ret > 0);

  for (auto table_name : table_names) {
    mdb::Schema* schema = new mdb::Schema();
    mdb::symbol_t symbol;
    sharding_->init_schema(table_name, schema, &symbol);
    mdb::Table* tb;

    switch (symbol) {
      case mdb::TBL_SORTED:
        tb = new mdb::SortedTable(table_name, schema);
        break;
      case mdb::TBL_UNSORTED:
        tb = new mdb::UnsortedTable(table_name, schema);
        break;
      case mdb::TBL_SNAPSHOT:
        tb = new mdb::SnapshotTable(table_name, schema);
        break;
      default:
        verify(0);
    }
    tx_sched_->reg_table(table_name, tb);
  }
  verify(sharding_);
  sharding_->PopulateTables(site_info_->partition_id_);
  Log_info("data populated for site: %x, partition: %x",
           site_info_->id, site_info_->partition_id_);
  verify(ret > 0);
}

void ServerWorker::RegisterWorkload() {
  Workload* workload = Workload::CreateWorkload(Config::GetConfig());
  verify(tx_reg_ != nullptr);
  verify(sharding_ != nullptr);
  workload->sss_ = sharding_;
  workload->txn_reg_ = tx_reg_;
  workload->RegisterPrecedures();
}

void ServerWorker::SetupService() {
  Log_info("enter %s for %s @ %s", __FUNCTION__,
           this->site_info_->name.c_str(),
           site_info_->GetBindAddress().c_str());

  int ret;
  // set running mode and initialize transaction manager.
  std::string bind_addr = site_info_->GetBindAddress();

  // init rrr::PollThreadWorker
  svr_poll_thread_worker_ = PollThreadWorker::create();
//  svr_thread_pool_ = new rrr::ThreadPool(1);

  // init service implementation

  if (tx_frame_ != nullptr) {
    services_ = tx_frame_->CreateRpcServices(site_info_->id,
                                             tx_sched_,
                                             svr_poll_thread_worker_,
                                             scsi_);
  }

  if (rep_frame_ != nullptr) {
    auto s2 = rep_frame_->CreateRpcServices(site_info_->id,
                                            rep_sched_,
                                            svr_poll_thread_worker_,
                                            scsi_);

    services_.insert(services_.end(), s2.begin(), s2.end());
  }

  // Setup shard master/kv services if we're running lab tests
  auto& yaml = Config::GetConfig()->yaml_config_;
  if (yaml["lab"].IsDefined() && yaml["lab"]["shard"].IsDefined() &&
      yaml["lab"]["shard"].as<bool>()) {
    if (this->site_info_->partition_id_ == 0) {
      verify(sm_svr_);
      services_.push_back(sm_svr_.get());
    } else {
      auto s = make_shared<ShardKvServiceImpl>();
      s->sp_svr_ = shardkv_svr_;
      verify(s);
      shardkv_service_ = s;  // Store to prevent premature deletion
      services_.push_back(s.get());
    }
  } else if (yaml["lab"].IsDefined() && yaml["lab"]["kv"].IsDefined() &&
             yaml["lab"]["kv"].as<bool>()) {
    auto s = make_shared<KvServiceImpl>();
    s->sp_svr_ = kv_svr_;
    verify(s);
    kv_service_ = s;  // Store to prevent premature deletion
    services_.push_back(s.get());
  }

//  auto& alarm = TimeoutALock::get_alarm_s();
//  ServerWorker::svr_poll_thread_worker_->add(&alarm);

  uint32_t num_threads = 1;
//  thread_pool_g = new base::ThreadPool(num_threads);

  // init rrr::Server
  rpc_server_ = new rrr::Server(svr_poll_thread_worker_, svr_thread_pool_);

  // reg services
  for (auto service : services_) {
    rpc_server_->reg(service);
  }

  // start rpc server
  Log_debug("starting server at %s", bind_addr.c_str());
  ret = rpc_server_->start(bind_addr.c_str());
  if (ret != 0) {
    Log_fatal("server launch failed.");
  }

  Log_info("Server %s ready at %s",
           site_info_->name.c_str(),
           bind_addr.c_str());

}

void ServerWorker::WaitForShutdown() {
  Log_debug("%s", __FUNCTION__);
  if (hb_rpc_server_ != nullptr) {
    scsi_->wait_for_shutdown();
    delete hb_rpc_server_;
    delete scsi_;
    // svr_hb_poll_thread_worker_g automatically released by shared_ptr
    if (hb_thread_pool_g != svr_thread_pool_)
      hb_thread_pool_g->release();

    for (auto service : services_) {
#ifdef CHECK_ISO
      this->tx_sched_->CheckDeltas();
#endif
      if (DepTranServiceImpl* s = dynamic_cast<DepTranServiceImpl*>(service)) {
        auto& recorder = s->recorder_;
        if (recorder) {
          auto n_flush_avg_ = recorder->stat_cnt_.peek().avg_;
          auto sz_flush_avg_ = recorder->stat_sz_.peek().avg_;
          Log::info("Log to disk, average log per flush: %lld,"
                        " average size per flush: %lld",
                    n_flush_avg_, sz_flush_avg_);
        }
      }
    }
  }
#ifdef CHECK_ISO
    for (auto service : services_) {
      this->tx_sched_->CheckDeltas();
    }
#endif

  Log_debug("exit %s", __FUNCTION__);
}

void ServerWorker::SetupCommo() {
  Log_info("SetupCommo: enter for site %d", site_info_->id);
  verify(svr_poll_thread_worker_);
  if (tx_frame_) {
    Log_info("SetupCommo: before CreateCommo(tx) for site %d", site_info_->id);
    tx_commo_ = tx_frame_->CreateCommo(svr_poll_thread_worker_);
    Log_info("SetupCommo: after CreateCommo(tx) for site %d", site_info_->id);
    if (tx_commo_) {
      tx_commo_->loc_id_ = site_info_->locale_id;
    }
    tx_sched_->commo_ = tx_commo_;
  }
  if (rep_frame_) {
    // Set KV/Shard server pointers for lab tests
    Log_info("SetupCommo: site %d setting rep_frame_->kv_svr_ = %p", site_info_->id, kv_svr_.get());
    rep_frame_->kv_svr_ = kv_svr_.get();
    rep_frame_->shardkv_svr_ = shardkv_svr_.get();
    rep_frame_->sm_svr_ = sm_svr_.get();

    Log_info("SetupCommo: before CreateCommo(rep) for site %d", site_info_->id);
    rep_commo_ = rep_frame_->CreateCommo(svr_poll_thread_worker_);
    Log_info("SetupCommo: after CreateCommo(rep) for site %d", site_info_->id);
    if (rep_commo_) {
      rep_commo_->loc_id_ = site_info_->locale_id;
    }
    rep_sched_->commo_ = rep_commo_;
    rep_commo_->rep_sched_ = rep_sched_;  // Critical: bidirectional link
  }

  Reactor::GetReactor()->server_id_ = site_info_->id;

  std::shared_ptr<rrr::OneTimeJob> sp_j = std::make_shared<rrr::OneTimeJob>(
    [this]() {
      if (rep_sched_) {
        rep_sched_->Setup();
        rep_sched_->setup_done_ = true;  // Signal that setup is complete
      }
    }
  );
  svr_poll_thread_worker_->add(sp_j);

#ifdef RAFT_TEST_CORO
  // Run test on a separate dedicated thread so it doesn't block any server's event loop
  if (rep_sched_ && rep_sched_->site_id_ == 0) {
    // Wait for Setup() to complete before starting test
    while (!rep_sched_->setup_done_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    Log_info("SetupCommo: Setup completed, launching test thread");

    // Launch test on separate thread
    std::thread test_thread([this]() {
      Log_info("Test thread started");
      // Create a new coroutine on this test thread that runs the test
      Coroutine::CreateRun([this]() {
        Log_info("Test coroutine started");
        verify(RaftFrame::all_sites_created_s);
        auto testconfig = new RaftTestConfig(RaftFrame::frames_);
        RaftLabTest test(testconfig);
        auto* raft_frame = dynamic_cast<RaftFrame*>(rep_frame_);
        Log_info("Test: raft_frame=%p", raft_frame);
        if (raft_frame) {
          Log_info("Test: raft_frame->kv_svr_=%p, raft_frame->sm_svr_=%p",
                   raft_frame->kv_svr_, raft_frame->sm_svr_);
          test.kv_svr_ = raft_frame->kv_svr_;
          test.sm_svr_ = raft_frame->sm_svr_;
        }
        Log_info("Test: test.kv_svr_=%p, test.sm_svr_=%p", test.kv_svr_, test.sm_svr_);
        Log_info("About to call test.Run()");
        test.Run();
        Log_info("test.Run() returned");
        test.Cleanup();
        Log_info("Test coroutine completed - exiting");
        exit(0);  // Exit when test completes
      });
      // Run reactor loop infinitely to process test coroutine events
      Log_debug("Starting infinite reactor loop for test thread");
      Reactor::GetReactor()->Loop(true, true);  // infinite=true, check_timeout=true
      Log_info("Test thread exiting");
    });
    test_thread.join(); // Wait for test to complete
    Log_info("Test thread joined");
  }
#endif
}

// NOTE: Pause/Resume/Slow are not supported by PollThreadWorker (only by old PollMgr)
// Commenting out for now - these were used for network simulation in tests
// void ServerWorker::Pause() {
//   svr_poll_thread_worker_->pause();
// }
//
// void ServerWorker::Slow(uint32_t sleep_usec) {
//   svr_poll_thread_worker_->slow(sleep_usec);
// }
//
// void ServerWorker::Resume() {
//   svr_poll_thread_worker_->resume();
// }

void ServerWorker::ShutDown() {
  Log_debug("deleting services, num: %d", services_.size());
  // Resume();  // Commented out - not supported by PollThreadWorker
  delete rpc_server_;
  for (auto service : services_) {
    delete service;
  }
//  thread_pool_g->release();
  // svr_poll_thread_worker_ automatically released by shared_ptr
}

int ServerWorker::DbChecksum() {
  auto cs = this->tx_sched_->mdb_txn_mgr_->Checksum();
  Log_info("site_id: %d shard_id: %d checksum: %x", (int)this->site_info_->id,
           (int)this->site_info_->partition_id_, (int) cs);
  return cs;
}

ServerWorker::~ServerWorker() {
  // Shutdown PollThreadWorkers if we own them
  if (svr_poll_thread_worker_) {
    svr_poll_thread_worker_->shutdown();
  }
  if (svr_hb_poll_thread_worker_g) {
    svr_hb_poll_thread_worker_g->shutdown();
  }
}

} // namespace janus

