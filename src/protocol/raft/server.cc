#include "server.h"
// #include "paxos_worker.h"
#include "exec.h"
#include "frame.h"
#include "coordinator.h"
#include "../classic/tpc_command.h"

namespace janus {

RaftServer::RaftServer(Frame* frame) {
  frame_ = frame;
  timer_ = new Timer();

  /* Lab skeleton: initialize any Raft state you need here.
   * This constructor can run in a different OS thread. */
  setIsLeader(false);
}

RaftServer::~RaftServer() {
  stop_ = true;
  if (timer_ != nullptr) {
    delete timer_;
    timer_ = nullptr;
  }

  /* Lab skeleton: add teardown logic if needed. */
}

void RaftServer::Setup() {
  /* Lab skeleton: start heartbeat/election coroutines here.
   * This function runs on the RPC thread and may race with incoming RPCs. */

  setup_done_ = true;
}

bool RaftServer::IsLeader() {
  return is_leader_;
}

void RaftServer::GetState(bool* is_leader, uint64_t* term) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  *is_leader = IsLeader();
  *term = currentTerm;
}

bool RaftServer::Start(shared_ptr<Marshallable>& cmd,
                       uint64_t* index,
                       uint64_t* term,
                       slotid_t slot_id,
                       ballot_t ballot) {
  (void) cmd;
  (void) slot_id;
  (void) ballot;

  /* Lab skeleton: if this server is leader, append command and start agreement.
   * Return false for non-leaders. */
  *index = 0;
  *term = 0;
  return false;
}

void RaftServer::SetLocalAppend(shared_ptr<Marshallable>& cmd,
                                uint64_t* term,
                                uint64_t* index,
                                slotid_t slot_id,
                                ballot_t ballot) {
  (void) cmd;
  (void) slot_id;
  (void) ballot;

  /* Lab skeleton helper: append one log entry locally.
   * Students can implement this helper or inline the logic in Start(). */
  *index = lastLogIndex;
  *term = currentTerm;
}

shared_ptr<RaftData> RaftServer::GetRaftInstance(slotid_t id) {
  verify(id >= min_active_slot_ || id == 0);
  auto& sp_instance = raft_logs_[id];
  if (!sp_instance) {
    sp_instance = std::make_shared<RaftData>();
  }
  return sp_instance;
}

void RaftServer::SyncRpcExample() {
  /* Optional example hook for synchronous RPC calls from a coroutine. */
}

bool RaftServer::RequestVote() {
  /* Lab skeleton: trigger leader election and request votes from peers. */
  return false;
}

void RaftServer::OnRequestVote(const slotid_t& lst_log_idx,
                               const ballot_t& lst_log_term,
                               const siteid_t& can_id,
                               const ballot_t& can_term,
                               ballot_t* reply_term,
                               bool_t* vote_granted,
                               const function<void()>& cb) {
  (void) lst_log_idx;
  (void) lst_log_term;
  (void) can_id;
  (void) can_term;

  /* Lab skeleton: apply Raft vote rules and fill RPC reply. */
  *reply_term = 0;
  *vote_granted = false;
  cb();
}

void RaftServer::OnAppendEntries(const slotid_t slot_id,
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
                                 const function<void()>& cb) {
  (void) slot_id;
  (void) ballot;
  (void) leaderCurrentTerm;
  (void) leaderPrevLogIndex;
  (void) leaderPrevLogTerm;
  (void) leaderCommitIndex;
  (void) cmd;
  (void) leaderNextLogTerm;

  /* Lab skeleton: validate and append entries, then update commit index. */
  *followerAppendOK = 0;
  *followerCurrentTerm = 0;
  *followerLastLogIndex = 0;
  cb();
}

void RaftServer::StartElectionTimer() {
  /* Lab skeleton: run randomized election timeout loop here. */
}

void RaftServer::HeartbeatLoop() {
  /* Lab skeleton: leader heartbeat / replication loop. */
}

void RaftServer::setIsLeader(bool is_leader) {
  is_leader_ = is_leader;
}

void RaftServer::applyLogs() {
  /* Lab skeleton: apply committed logs to app_next_ in order. */
}

void RaftServer::resetTimerBatch() {
  if (!failover_) {
    return;
  }
  auto cur_count = counter_++;
  if (cur_count > NUM_BATCH_TIMER_RESET) {
    if (timer_ != nullptr && timer_->elapsed() > SEC_BATCH_TIMER_RESET) {
      resetTimer();
    }
    counter_.store(0);
  }
}

void RaftServer::resetTimer() {
  last_heartbeat_time_ = Time::now();
  if (failover_ && timer_ != nullptr) {
    timer_->start();
  }
}

double RaftServer::randDuration() {
  return RandomGenerator::rand_double(0.4, 0.7);
}

/* Do not modify any code below here */

void RaftServer::Disconnect(const bool disconnect) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  verify(disconnected_ != disconnect);
  // Global map of rpc_par_proxies_ values accessed by partition then by site.
  static map<parid_t, map<siteid_t, map<siteid_t, vector<SiteProxyPair>>>> _proxies{};
  if (_proxies.find(partition_id_) == _proxies.end()) {
    _proxies[partition_id_] = {};
  }
  RaftCommo* c = (RaftCommo*) commo();
  if (disconnect) {
    verify(_proxies[partition_id_][loc_id_].size() == 0);
    verify(c->rpc_par_proxies_.size() > 0);
    auto sz = c->rpc_par_proxies_.size();
    _proxies[partition_id_][loc_id_].insert(c->rpc_par_proxies_.begin(), c->rpc_par_proxies_.end());
    c->rpc_par_proxies_ = {};
    verify(_proxies[partition_id_][loc_id_].size() == sz);
    verify(c->rpc_par_proxies_.size() == 0);
  } else {
    verify(_proxies[partition_id_][loc_id_].size() > 0);
    auto sz = _proxies[partition_id_][loc_id_].size();
    c->rpc_par_proxies_ = {};
    c->rpc_par_proxies_.insert(_proxies[partition_id_][loc_id_].begin(), _proxies[partition_id_][loc_id_].end());
    _proxies[partition_id_][loc_id_] = {};
    verify(_proxies[partition_id_][loc_id_].size() == 0);
    verify(c->rpc_par_proxies_.size() == sz);
  }
  disconnected_ = disconnect;
}

void RaftServer::Reconnect() {
  Disconnect(false);
  resetTimer();
}

bool RaftServer::IsDisconnected() {
  return disconnected_;
}

void RaftServer::removeCmd(slotid_t slot) {
  raft_logs_.erase(slot);
}

}  // namespace janus
