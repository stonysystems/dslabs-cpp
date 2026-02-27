#include "__dep__.h"
#include "frame.h"
#include "config.h"
// Slim build: RCC and Snow protocols excluded
// #include "rcc/row.h"
// #include "snow/ro6_row.h"
#include "marshal-value.h"
#include "coordinator.h"
#include "tx.h"
#include "2pl/tx.h"
#include "service.h"
#include "scheduler.h"
#include "none/coordinator.h"
#include "none/scheduler.h"
// Slim build: RCC and Snow protocols excluded
// #include "rcc/coord.h"
// #include "snow/ro6_coord.h"
#include "2pl/coordinator.h"
#include "occ/tx.h"
#include "occ/coordinator.h"

#include "protocol/2pl/scheduler.h"
#include "occ/scheduler.h"
#include "workload.h"

#include "extern_c/frame.h"


namespace janus {

namespace {

class LabSharding final : public Sharding {
 public:
  void PreparePrimaryColumn(tb_info_t* tb_info,
                            uint32_t col_index,
                            mdb::Schema::iterator& col_it) override {
    (void) tb_info;
    (void) col_index;
    (void) col_it;
    verify(0);
  }

  bool GenerateRowData(tb_info_t* tb_info,
                       uint32_t& sid,
                       Value& key_value,
                       vector<Value>& row_data) override {
    (void) tb_info;
    (void) sid;
    (void) key_value;
    (void) row_data;
    verify(0);
    return false;
  }

  void InsertRowData(tb_info_t* tb_info,
                     uint32_t& partition_id,
                     Value& key_value,
                     const mdb::Schema* schema,
                     mdb::Table* const table_ptr,
                     mdb::SortedTable* tbl_sec_ptr,
                     vector<Value>& row_data) override {
    (void) tb_info;
    (void) partition_id;
    (void) key_value;
    (void) schema;
    (void) table_ptr;
    (void) tbl_sec_ptr;
    (void) row_data;
    verify(0);
  }
};

} // namespace

Frame* Frame::RegFrame(int mode,
                       function<Frame*()> frame_init) {
  auto& mode_to_frame = Frame::ModeToFrame();
  auto it = mode_to_frame.find(mode);
  verify(it == mode_to_frame.end());
  mode_to_frame[mode] = frame_init;
  return frame_init();
}

Frame* Frame::GetFrame(int mode) {
  Frame *frame = nullptr;
  // some built-in mode
  switch (mode) {
    case MODE_NONE:
    case MODE_MDCC:
    case MODE_2PL:
    case MODE_OCC:
      frame = new Frame(mode);
      break;
    case MODE_EXTERNC:
      frame = new ExternCFrame();
      break;
    default:
      auto& mode_to_frame = Frame::ModeToFrame();
      auto it = mode_to_frame.find(mode);
      verify(it != mode_to_frame.end());
      frame = it->second();
  }

  return frame;
}

int Frame::Name2Mode(string name) {
  auto &m = Frame::FrameNameToMode();
  auto it = m.find(name);
  verify(it != m.end());
  return it->second;
}

Frame* Frame::GetFrame(string name) {
  return GetFrame(Name2Mode(name));
}

Frame* Frame::RegFrame(int mode,
                       vector<string> names,
                       function<Frame*()> frame) {
  for (auto name: names) {
    //verify(frame_name_mode_s.find(name) == frame_name_mode_s.end());
    auto &m = Frame::FrameNameToMode();
    m[name] = mode;
  }
  return RegFrame(mode, frame);
}

Sharding* Frame::CreateSharding() {
  auto bench = Config::config_s->benchmark_;
  if (bench == BENCH_NONE) {
    return new LabSharding();
  }

  Log_fatal("src/bench workloads are disabled in this lab-only tree");
  verify(0);
  return nullptr;
}

Sharding* Frame::CreateSharding(Sharding *sd) {
  verify(sd != nullptr);
  Sharding* ret = CreateSharding();
  *ret = *sd;
  ret->frame_ = this;
  return ret;
}

mdb::Row* Frame::CreateRow(const mdb::Schema *schema,
                           vector<Value> &row_data) {
//  auto mode = Config::GetConfig()->cc_mode_;
  auto mode = mode_;
  mdb::Row* r = nullptr;
  switch (mode) {
    case MODE_2PL:
      r = mdb::FineLockedRow::create(schema, row_data);
      break;
    // Slim build: Snow protocol excluded
    // case MODE_RO6:
    //   r = RO6Row::create(schema, row_data);
    //   break;
    case MODE_NONE: // FIXME
    case MODE_MDCC:
    case MODE_OCC:
    default:
      r = mdb::VersionedRow::create(schema, row_data);
      break;
  }
  return r;
}

Coordinator* Frame::CreateCoordinator(cooid_t coo_id,
                                      Config *config,
                                      int benchmark,
                                      ClientControlServiceImpl *ccsi,
                                      uint32_t id,
                                      shared_ptr<TxnRegistry> txn_reg) {
  // TODO: clean this up; make Coordinator subclasses assign txn_reg_
  Coordinator *coo;
  auto attr = this;
//  auto mode = Config::GetConfig()->cc_mode_;
  auto mode = mode_;
  switch (mode) {
    case MODE_2PL:
      coo = new Coordinator2pl(coo_id,
                         benchmark,
                         ccsi,
                         id);
      ((Coordinator*)coo)->txn_reg_ = txn_reg;
      break;
    case MODE_OCC:
    case MODE_RPC_NULL:
      coo = new CoordinatorOcc(coo_id,
                         benchmark,
                         ccsi,
                         id);
      ((Coordinator*)coo)->txn_reg_ = txn_reg;
      break;
    // Slim build: RCC and Snow protocols excluded
    // case MODE_RCC:
    //   coo = new RccCoord(coo_id,
    //                      benchmark,
    //                      ccsi,
    //                      id);
    //   ((Coordinator*)coo)->txn_reg_ = txn_reg;
    //   break;
    // case MODE_RO6:
    //   coo = new RO6Coord(coo_id,
    //                      benchmark,
    //                      ccsi,
    //                      id);
    //   ((Coordinator*)coo)->txn_reg_ = txn_reg;
    //   break;
    case MODE_MDCC:
//      coo = (Coordinator*)new mdcc::MdccCoordinator(coo_id, id, config, ccsi);
      break;
    case MODE_NONE:
    default:
      coo = new CoordinatorNone(coo_id,
                          benchmark,
                          ccsi,
                          id);
      ((Coordinator*)coo)->txn_reg_ = txn_reg;
      break;
  }
  coo->frame_ = this;
  return coo;
}

Coordinator* Frame::CreateBulkCoordinator(Config *config, int benchmark) {
  verify(0);
  Coordinator *coo;
  return coo;
}

void Frame::GetTxTypes(std::map<int32_t, std::string>& txn_types) {
  txn_types.clear();
  if (Config::config_s->benchmark_ == BENCH_NONE) {
    return;
  }

  Log_fatal("src/bench workloads are disabled in this lab-only tree");
  verify(0);
}

TxData* Frame::CreateTxnCommand(TxRequest& req, shared_ptr<TxnRegistry> reg) {
  (void) req;
  (void) reg;
  auto benchmark = Config::config_s->benchmark_;
  if (benchmark == BENCH_NONE) {
    Log_fatal("No transaction command generator exists for BENCH_NONE");
    verify(0);
  }
  Log_fatal("src/bench workloads are disabled in this lab-only tree");
  verify(0);
  return nullptr;
}

//TxData * Frame::CreateChopper(TxRequest &req, TxnRegistry* reg) {
//  return CreateTxnCommand(req, reg);
//}

Communicator* Frame::CreateCommo(rusty::Arc<PollThreadWorker> poll_thread_worker) {
  commo_ = new Communicator(poll_thread_worker);
  return commo_;
}

shared_ptr<Tx> Frame::CreateTx(epoch_t epoch, txnid_t tid,
                               bool ro, TxLogServer *mgr) {
  shared_ptr<Tx> sp_tx;

  switch (mode_) {
    case MODE_2PL:
      sp_tx.reset(new Tx2pl(epoch, tid, mgr));
      break;
    case MODE_OCC:
      sp_tx.reset(new TxOcc(epoch, tid, mgr));
      break;
    // Slim build: RCC and Snow protocols excluded
    // case MODE_RCC:
    //   sp_tx.reset(new RccTx(epoch, tid, mgr, ro));
    //   break;
    // case MODE_RO6:
    //   sp_tx.reset(new TxSnow(tid, mgr, ro));
    //   break;
    case MODE_MULTI_PAXOS:
      break;
    case MODE_NONE:
    default:
      sp_tx.reset(new Tx2pl(epoch, tid, mgr));
      break;
  }
  return sp_tx;
}

Executor* Frame::CreateExecutor(cmdid_t cmd_id, TxLogServer* sched) {
  Executor* exec = nullptr;
//  auto mode = Config::GetConfig()->cc_mode_;
//  switch (mode) {
//    case MODE_NONE:
//      verify(0);
//    case MODE_2PL:
//      exec = new TplExecutor(cmd_id, sched);
//      break;
//    case MODE_OCC:
//      exec = new OCCExecutor(cmd_id, sched);
//      break;
//    default:
//      verify(0);
//  }
  return exec;
}

TxLogServer* Frame::CreateScheduler() {
  auto mode = Config::GetConfig()->tx_proto_;
  TxLogServer *sch = nullptr;
  switch(mode) {
    case MODE_2PL:
      sch = new Scheduler2pl();
      break;
    case MODE_OCC:
      sch = new SchedulerOcc();
      break;
    case MODE_MDCC:
//      sch = new mdcc::MdccScheduler();
      break;
    case MODE_NONE:
      sch = new SchedulerNone();
      break;
    case MODE_RPC_NULL:
    // Slim build: RCC and Snow protocols excluded
    // case MODE_RCC:
    // case MODE_RO6:
    //   verify(0);
    //   break;
    default:
      verify(0);
//      sch = new CustomSched();
  }
  verify(sch);
  sch->frame_ = this;
  return sch;
}

Workload * Frame::CreateTxGenerator() {
  auto benchmark = Config::config_s->benchmark_;
  if (benchmark == BENCH_NONE) {
    return Workload::CreateWorkload(Config::GetConfig());
  }
  Log_fatal("src/bench workloads are disabled in this lab-only tree");
  verify(0);
  return nullptr;
}

vector<rrr::Service *> Frame::CreateRpcServices(uint32_t site_id,
                                                TxLogServer *dtxn_sched,
                                                rusty::Arc<rrr::PollThreadWorker> poll_thread_worker,
                                                ServerControlServiceImpl *scsi) {
  auto config = Config::GetConfig();
  auto result = std::vector<Service *>();
  switch(mode_) {
    case MODE_MDCC:
    case MODE_2PL:
    case MODE_OCC:
    case MODE_NONE:
    // Slim build: excluded protocols
    // case MODE_TAPIR:
    // case MODE_JANUS:
    // case MODE_RCC:
    default:
      result.push_back(new ClassicServiceImpl(dtxn_sched, poll_thread_worker, scsi));
      break;
  }
  return result;
}
map<string, int> &Frame::FrameNameToMode() {
  static map<string, int> frame_name_mode_s = {
      {"none",          MODE_NONE},
      {"2pl",           MODE_2PL},
      {"occ",           MODE_OCC},
      {"snow",           MODE_RO6},
      {"rpc_null",      MODE_RPC_NULL},
      {"deptran",       MODE_DEPTRAN},
      {"deptran_er",    MODE_DEPTRAN},
      {"2pl_w",         MODE_2PL},
      {"2pl_wait_die",  MODE_2PL},
      {"2pl_wd",        MODE_2PL},
      {"2pl_ww",        MODE_2PL},
      {"2pl_wound_die", MODE_2PL},
      {"externc",       MODE_EXTERNC},
      {"extern_c",      MODE_EXTERNC},
      {"mdcc",          MODE_MDCC},
      {"multi_paxos",   MODE_MULTI_PAXOS},
      {"epaxos",        MODE_NOT_READY},
      {"rep_commit",    MODE_NOT_READY}
  };
  return frame_name_mode_s;
}

map<int, function<Frame*()>> &Frame::ModeToFrame() {
  static map<int, function<Frame*()>> frame_s_ = {};
  return frame_s_;
}
} // namespace janus;
