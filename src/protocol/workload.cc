
#include "workload.h"

#include "config.h"
#include "constants.h"
#include "sharding.h"

namespace janus {

namespace {

class NoopWorkload final : public Workload {
 public:
  explicit NoopWorkload(Config* config) : Workload(config) {}

  void GetTxRequest(TxRequest* req, uint32_t cid) override {
    (void) req;
    (void) cid;
    Log_fatal("No benchmark workload configured");
    verify(0);
  }

  void RegisterPrecedures() override {
    // Lab-only mode: no transactional benchmark procedures are registered.
  }
};

} // namespace

Workload* Workload::CreateWorkload(Config *config) {
  if (config->benchmark() == BENCH_NONE) {
    return new NoopWorkload(config);
  }
  Log_fatal("src/bench workloads are disabled in this lab-only tree");
  verify(0);
  return nullptr;
}

Workload::Workload(Config* config)
    : txn_weight_(config->get_txn_weight()),
      txn_weights_(config->get_txn_weights()),
      sharding_(config->sharding_) {
  benchmark_ = Config::GetConfig()->benchmark();
  n_try_ = Config::GetConfig()->get_max_retry();
  single_server_ = Config::GetConfig()->get_single_server();

  std::map<std::string, uint64_t> table_num_rows;
  sharding_->get_number_rows(table_num_rows);

  if (Config::GetConfig()->dist_ == "fixed") {
    single_server_ = Config::SS_PROCESS_SINGLE;
  }

  if (benchmark_ == BENCH_NONE) {
    return;
  }

  Log_fatal("src/bench workloads are disabled in this lab-only tree");
  verify(0);
}

void Workload::GetProcedureTypes(map<int32_t, string> &txn_types) {
  txn_types.clear();
  if (benchmark_ == BENCH_NONE) {
    return;
  }

  Log_fatal("src/bench workloads are disabled in this lab-only tree");
  verify(0);
}

Workload::~Workload() {
}

} // namespace janus
