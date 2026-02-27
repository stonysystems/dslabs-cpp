#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../scheduler.h"
#include "../classic/tpc_command.h"
#include "commo.h"

namespace janus {
class Command;
class CmdData;

#define INVALID_SITEID  ((siteid_t)-1)
#define NUM_BATCH_TIMER_RESET  (100)
#define SEC_BATCH_TIMER_RESET  (1)

struct RaftData {
  ballot_t max_ballot_seen_ = 0;
  ballot_t max_ballot_accepted_ = 0;
  shared_ptr<Marshallable> accepted_cmd_{nullptr};
  shared_ptr<Marshallable> committed_cmd_{nullptr};

  ballot_t term;
  shared_ptr<Marshallable> log_{nullptr};

  ballot_t prevTerm;
  slotid_t slot_id;
  ballot_t ballot;
};

struct KeyValue {
  int key;
  i32 value;
};

#define HEARTBEAT_INTERVAL 100000

class RaftServer : public TxLogServer {
 private:
#ifdef RAFT_USE_RUST
  // Opaque pointer to Rust RaftServer instance.
  void* rust_server_ptr_ = nullptr;
#endif

  std::map<siteid_t, uint64_t> match_index_{};
  std::map<siteid_t, uint64_t> next_index_{};
  std::vector<std::thread> timer_threads_ = {};
  Timer* timer_ = nullptr;
  uint64_t last_heartbeat_time_ = 0;
  bool stop_ = false;
  siteid_t vote_for_ = INVALID_SITEID;
  bool init_ = false;
  bool is_leader_ = false;
  slotid_t snapidx_ = 0;
  ballot_t snapterm_ = 0;
  int32_t wait_int_ = 100000;
  bool disconnected_ = false;
  bool req_voting_ = false;
  bool in_applying_logs_ = false;
#ifdef RAFT_TEST_CORO
  bool failover_{true};
#else
  bool failover_{false};
#endif
  atomic<int64_t> counter_{0};

  bool looping_ = false;
  bool heartbeat_ = true;

  bool RequestVote();
  void HeartbeatLoop();
  RaftCommo* commo() { return (RaftCommo*) commo_; }
  void setIsLeader(bool isLeader);
  void applyLogs();
  void resetTimerBatch();
  void resetTimer();
  double randDuration();

 public:
  slotid_t min_active_slot_ = 1;  // anything before this slot is freed.
  slotid_t max_executed_slot_ = 0;
  slotid_t max_committed_slot_ = 0;
  map<slotid_t, shared_ptr<RaftData>> logs_{};
  int n_vote_ = 0;
  int n_prepare_ = 0;
  int n_accept_ = 0;
  int n_commit_ = 0;

  // Core Raft state used by both C++ and Rust implementations.
  uint64_t lastLogIndex = 0;
  uint64_t currentTerm = 0;
  uint64_t commitIndex = 0;
  uint64_t executeIndex = 0;
  map<slotid_t, shared_ptr<RaftData>> raft_logs_{};

  RaftServer(Frame* frame);
  ~RaftServer();

  void Setup();
  void StartElectionTimer();
  void SyncRpcExample();

  bool IsLeader();

  bool Start(shared_ptr<Marshallable>& cmd,
             uint64_t* index,
             uint64_t* term,
             slotid_t slot_id = -1,
             ballot_t ballot = 1);

  void GetState(bool* is_leader, uint64_t* term);

  void SetLocalAppend(shared_ptr<Marshallable>& cmd,
                      uint64_t* term,
                      uint64_t* index,
                      slotid_t slot_id = -1,
                      ballot_t ballot = 1);

  shared_ptr<RaftData> GetInstance(slotid_t id) {
    verify(id >= min_active_slot_ || lastLogIndex == 0);
    auto& sp_instance = logs_[id];
    if (!sp_instance) {
      sp_instance = std::make_shared<RaftData>();
    }
    return sp_instance;
  }

  shared_ptr<RaftData> GetRaftInstance(slotid_t id);

  void OnRequestVote(const slotid_t& lst_log_idx,
                     const ballot_t& lst_log_term,
                     const siteid_t& can_id,
                     const ballot_t& can_term,
                     ballot_t* reply_term,
                     bool_t* vote_granted,
                     const function<void()>& cb);

  void OnAppendEntries(const slotid_t slot_id,
                       const ballot_t ballot,
                       const uint64_t leaderCurrentTerm,
                       const uint64_t leaderPrevLogIndex,
                       const uint64_t leaderPrevLogTerm,
                       const uint64_t leaderCommitIndex,
                       shared_ptr<Marshallable>& cmd,
                       const uint64_t leaderNextLogTerm,
                       uint64_t* followerAppendOK,
                       uint64_t* followerCurrentTerm,
                       uint64_t* followerLastLogIndex,
                       const function<void()>& cb);

  void Disconnect(const bool disconnect = true);
  void Reconnect();
  bool IsDisconnected();

  virtual bool HandleConflicts(Tx& dtxn,
                               innid_t inn_id,
                               vector<string>& conflicts) {
    verify(0);
  }

  void removeCmd(slotid_t slot);
};
}  // namespace janus
