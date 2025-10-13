#include "labtestconf.h"
#include "../src/deptran/marshallable.h"
#include "../src/deptran/raft/server.h"  // For HEARTBEAT_INTERVAL
#include <chrono>
#include <thread>

namespace janus {

#ifdef RAFT_TEST_CORO

int _test_id_g = 0;

// RaftFrame **RaftTestConfig::replicas = nullptr;
map<siteid_t, RaftFrame*> RaftTestConfig::frames = {};
std::function<void(Marshallable &)> RaftTestConfig::commit_callbacks[NSERVERS];
std::vector<int> RaftTestConfig::committed_cmds[NSERVERS];
uint64_t RaftTestConfig::rpc_count_last[NSERVERS];

RaftTestConfig::RaftTestConfig(map<siteid_t, RaftFrame*>& f) {
  auto& frames = RaftTestConfig::frames;
  verify(frames.empty());
  frames = f;
  for (int i = 0; i < NSERVERS; i++) {
    frames[i]->svr_->rep_frame_ = frames[i]->svr_->frame_;
    RaftTestConfig::committed_cmds[i].push_back(-1);
    RaftTestConfig::rpc_count_last[i] = 0;
    disconnected_[i] = false;
  }
  th_ = std::thread([this](){ netctlLoop(); });
}

void RaftTestConfig::SetLearnerAction(void) {
  for (int i = 0; i < NSERVERS; i++) {
    // Create a lambda with the expected signature for RegLearnerAction
    auto learner_action = [i](int slot, shared_ptr<Marshallable> cmd) -> int {
      if (cmd) {
        verify(cmd->kind_ == MarshallDeputy::CMD_TPC_COMMIT);
        auto& command = dynamic_cast<TpcCommitCommand&>(*cmd);
        Log_debug("server %d committed value %d at slot %d", i, command.tx_id_, slot);
        
        // Verify that the slot matches the expected array index
        auto expected_index = RaftTestConfig::committed_cmds[i].size();
        if (slot != expected_index) {
          Log_fatal("SLOT MISMATCH: server %d, slot=%d, expected_index=%lu, committed_cmds.size()=%lu", 
                    i, slot, expected_index, RaftTestConfig::committed_cmds[i].size());
        }
        verify(slot == expected_index);
        
        RaftTestConfig::committed_cmds[i].push_back(command.tx_id_);
        
        // Also update the old callback for compatibility if needed
        if (RaftTestConfig::commit_callbacks[i]) {
          RaftTestConfig::commit_callbacks[i](*cmd);
        }
      }
      return 0;
    };
    RaftTestConfig::frames[i]->svr_->RegLearnerAction(learner_action);
  }
}

int RaftTestConfig::OneLeader(int expected) {
  return waitOneLeader(true, expected);
}

bool RaftTestConfig::NoLeader(void) {
  int r = waitOneLeader(false, -1);
  return r == -1;
}

int RaftTestConfig::waitOneLeader(bool want_leader, int expected) {
  uint64_t mostRecentTerm = 0, term;
  int leader = -1, i, retry;
  bool isleader;
  Log_debug("waitOneLeader: start, want_leader=%d, expected=%d", want_leader, expected);
  for (retry = 0; retry < 10; retry++) {
    // Reactor::CreateSpEvent<TimeoutEvent>(ELECTIONTIMEOUT / 10)->Wait();
    // Coroutine::Sleep(ELECTIONTIMEOUT/10);
    usleep(ELECTIONTIMEOUT/10);
    leader = -1;
    mostRecentTerm = 0;
    for (i = 0; i < NSERVERS; i++) {
      // ignore disconnected servers
      if (RaftTestConfig::frames[i]->svr_->IsDisconnected())
        continue;
      RaftTestConfig::frames[i]->svr_->GetState(&isleader, &term);
      Log_debug("waitOneLeader: retry %d, server %d: isleader=%d, term=%lu", retry, i, isleader, term);
      if (isleader) {
        if (term == mostRecentTerm) {
          Failed("multiple leaders elected in term %ld", term);
          return -2;
        } else if (term > mostRecentTerm) {
          leader = i;
          mostRecentTerm = term;
          Log_debug("waitOneLeader: found leader %d with term %lu", leader, term);
        }
      }
    }
    if (leader != -1) {
      Log_debug("waitOneLeader: found leader %d, sleeping to allow heartbeat propagation", leader);
      // Sleep for 2-3 heartbeats to let the leader update followers' terms
      // This is less than the original 1 second but enough for term synchronization
      usleep(3 * HEARTBEAT_INTERVAL);
      Log_debug("waitOneLeader: returning leader %d", leader);
      if (!want_leader) {
        Failed("leader elected despite lack of quorum");
      } else if (expected >= 0 && leader != expected) {
        Failed("unexpected leader change, expecting %d, got %d", expected, leader);
        return -3;
      }
      return leader;
    }
    Log_debug("waitOneLeader: retry %d, no leader found yet", retry);
  }
  if (want_leader) {
    Log_debug("waitOneLeader: timeout, no leader found");
    Failed("waited too long for leader election");
  }
  return -1;
}

bool RaftTestConfig::TermMovedOn(uint64_t term) {
  for (int i = 0; i < NSERVERS; i++) {
    uint64_t curTerm;
    bool isLeader;
    RaftTestConfig::frames[i]->svr_->GetState(&isLeader, &curTerm);
    if (curTerm > term) {
      return true;
    }
  }
  return false;
}

uint64_t RaftTestConfig::OneTerm(void) {
  uint64_t term, curTerm;
  bool isLeader;
  RaftTestConfig::frames[0]->svr_->GetState(&isLeader, &term);
  Log_debug("OneTerm: server 0 has term %lu", term);
  for (int i = 1; i < NSERVERS; i++) {
    RaftTestConfig::frames[i]->svr_->GetState(&isLeader, &curTerm);
    Log_debug("OneTerm: server %d has term %lu", i, curTerm);
    if (curTerm != term) {
      Log_debug("OneTerm: DISAGREEMENT - server 0 term=%lu, server %d term=%lu", term, i, curTerm);
      return -1;
    }
  }
  return term;
}

int RaftTestConfig::NCommitted(uint64_t index) {
  int cmd, n = 0;
  for (int i = 0; i < NSERVERS; i++) {
    if (RaftTestConfig::committed_cmds[i].size() > index) {
      auto curcmd = RaftTestConfig::committed_cmds[i][index];
      if (n == 0) {
        cmd = curcmd;
      } else {
        if (curcmd != cmd) {
          return -1;
        }
      }
      n++;
    }
  }
  return n;
}

bool RaftTestConfig::Start(int svr, int cmd, uint64_t *index, uint64_t *term) {
  // Construct an empty TpcCommitCommand containing cmd as its tx_id_
  auto cmdptr = std::make_shared<TpcCommitCommand>();
  auto vpd_p = std::make_shared<VecPieceData>();
  vpd_p->sp_vec_piece_data_ = std::make_shared<vector<shared_ptr<SimpleCommand>>>();
  cmdptr->tx_id_ = cmd;
  cmdptr->cmd_ = vpd_p;
  auto cmdptr_m = dynamic_pointer_cast<Marshallable>(cmdptr);
  // call Start()
  Log_debug("Starting agreement on svr %d for cmd id %d", svr, cmdptr->tx_id_);
  return RaftTestConfig::frames[svr]->svr_->Start(cmdptr_m, index, term);
}

int RaftTestConfig::Wait(uint64_t index, int n, uint64_t term) {
  int nc = 0, i;
  auto to = 10000; // 10 milliseconds
  for (i = 0; i < 30; i++) {
    nc = NCommitted(index);
    if (nc < 0) {
      return -3; // values differ
    } else if (nc >= n) {
      break;
    }
    // Use usleep instead of Reactor sleep since this can be called from pthread threads
    usleep(to);
    if (to < 1000000) {
      to *= 2;
    }
    if (TermMovedOn(term)) {
      return -2; // term changed
    }
  }
  if (i == 30) {
    return -1; // timeout
  }
  for (int i = 0; i < NSERVERS; i++) {
    if (RaftTestConfig::committed_cmds[i].size() > index) {
      return RaftTestConfig::committed_cmds[i][index];
    }
  }
  verify(0);
}

uint64_t RaftTestConfig::DoAgreement(int cmd, int n, bool retry) {
  Log_debug("Doing 1 round of Raft agreement");
  auto start = chrono::steady_clock::now();
  while ((chrono::steady_clock::now() - start) < chrono::seconds{10}) {
    usleep(50000);
    // Coroutine::Sleep(50000);
    // Call Start() to all servers until leader is found
    int ldr = -1;
    uint64_t index, term;
    for (int i = 0; i < NSERVERS; i++) {
      // skip disconnected servers
      if (RaftTestConfig::frames[i]->svr_->IsDisconnected())
        continue;
      if (Start(i, cmd, &index, &term)) {
        Log_debug("starting cmd ldr=%d cmd=%d index=%ld term=%ld", 
            RaftTestConfig::frames[i]->svr_->loc_id_, cmd, index, term);
        ldr = i;
        break;
      }
    }
    if (ldr != -1) {
      // If Start() successfully called, wait for agreement
      auto start2 = chrono::steady_clock::now();
      int nc;
      while ((chrono::steady_clock::now() - start2) < chrono::seconds{2}) {
        nc = NCommitted(index);
        if (nc < 0) {
          break;
        } else if (nc >= n) {
          for (int i = 0; i < NSERVERS; i++) {
            if (RaftTestConfig::committed_cmds[i].size() > index) {
              Log_debug("found commit log");
              auto cmd2 = RaftTestConfig::committed_cmds[i][index];
              if (cmd == cmd2) {
                return index;
              }
              break;
            }
          }
          break;
        }
        usleep(20000);
        // Coroutine::Sleep(50000);
      }
      Log_debug("%d committed server at index %d", nc, index);
      if (!retry) {
          Log_debug("failed to reach agreement");
          return 0;
        }
    } else {
      // If no leader found, sleep and retry.
      usleep(50000);
      // Coroutine::Sleep(50000);
    }
  }
  Log_debug("Failed to reach agreement end");
  return 0;
}

shared_ptr<CommitIndex> RaftTestConfig::StartAgreement(int svr, int cmd) {
  verify(0); // this function has been replaced by Start()
  auto cmt_idx_p = std::make_shared<CommitIndex>(0);
  std::shared_ptr<OneTimeJob> sp_otj = std::make_shared<OneTimeJob>(
    [this, cmd, svr, cmt_idx_p]() {
      auto cmdptr = std::make_shared<TpcCommitCommand>();
      auto vpd_p = std::make_shared<VecPieceData>();
      vpd_p->sp_vec_piece_data_ = std::make_shared<vector<shared_ptr<SimpleCommand>>>();
      cmdptr->tx_id_ = cmd;
      cmdptr->cmd_ = vpd_p;
      Log_debug("Starting agreement for cmd id %d", cmdptr->tx_id_);
      auto cmdptr_m = dynamic_pointer_cast<Marshallable>(cmdptr);
      RaftTestConfig::frames[svr]->svr_->CreateRepCoord(0)->Submit(cmdptr_m, [svr, cmt_idx_p](){
        cmt_idx_p->setval(RaftTestConfig::frames[svr]->svr_->commitIndex);
      });
    }
  );
  auto sp_job = std::dynamic_pointer_cast<Job>(sp_otj);
  RaftTestConfig::frames[svr]->commo_->rpc_poll_->add(sp_job);
  Log_debug("Started agreement for cmd id %d", cmd);
  return cmt_idx_p;
}

void RaftTestConfig::Disconnect(int svr) {
  verify(svr >= 0 && svr < NSERVERS);
  std::lock_guard<std::mutex> lk(disconnect_mtx_);
  verify(!disconnected_[svr]);
  disconnect(svr, true);
  disconnected_[svr] = true;
}

void RaftTestConfig::Reconnect(int svr) {
  verify(svr >= 0 && svr < NSERVERS);
  std::lock_guard<std::mutex> lk(disconnect_mtx_);
  verify(disconnected_[svr]);
  reconnect(svr);
  disconnected_[svr] = false;
}

int RaftTestConfig::NDisconnected(void) {
  int count = 0;
  for (int i = 0; i < NSERVERS; i++) {
    if (disconnected_[i])
      count++;
  }
  return count;
}

void RaftTestConfig::SetUnreliable(bool unreliable) {
  Log_debug("SetUnreliable(%d): Called, about to acquire cv_m_", unreliable);
  std::unique_lock<std::mutex> lk(cv_m_);
  Log_debug("SetUnreliable(%d): Acquired cv_m_, finished_=%d", unreliable, finished_);
  verify(!finished_);
  if (unreliable) {
    verify(!unreliable_);
    // lk acquired cv_m_ in state 1 or 0
    Log_debug("SetUnreliable(true): Setting unreliable_ = true");
    unreliable_ = true;
    // if cv_m_ was in state 1, must signal cv_ to wake up netctlLoop
    lk.unlock();
    Log_debug("SetUnreliable(true): Released cv_m_, notifying netctlLoop");
    cv_.notify_one();
    Log_debug("SetUnreliable(true): Done");
  } else {
    verify(unreliable_);
    // lk acquired cv_m_ in state 2 or 0
    Log_debug("SetUnreliable(false): Setting unreliable_ = false");
    unreliable_ = false;
    // wait until netctlLoop moves cv_m_ from state 2 (or 0) to state 1,
    // restoring the network to reliable state in the process.
    lk.unlock();
    Log_debug("SetUnreliable(false): Released cv_m_, returning");
    // Give netctlLoop time to see unreliable_ = false and clean up
    // netctlLoop runs every 100ms, so wait at least that long
    // usleep(200000); // 200ms to be safe
    // lk.lock();
  }
  Log_debug("SetUnreliable(%d): Function complete", unreliable);
}

bool RaftTestConfig::IsUnreliable(void) {
  return unreliable_;
}

void RaftTestConfig::Shutdown(void) {
  // trigger netctlLoop shutdown
  {
    std::unique_lock<std::mutex> lk(cv_m_);
    verify(!finished_);
    // lk acquired cv_m_ in state 0, 1, or 2
    finished_ = true;
    // if cv_m_ was in state 1, must signal cv_ to wake up netctlLoop
    lk.unlock();
    cv_.notify_one();
  }
  // wait for netctlLoop thread to exit
  th_.join();
  // Reconnect() all Deconnect()ed servers
  for (int i = 0; i < NSERVERS; i++) {
    if (disconnected_[i]) {
      Reconnect(i);
    }
  }
}

uint64_t RaftTestConfig::RpcCount(int svr, bool reset) {
  std::lock_guard<std::recursive_mutex> lk(
    RaftTestConfig::frames[svr]->commo_->rpc_mtx_);
  uint64_t count = RaftTestConfig::frames[svr]->commo_->rpc_count_;
  uint64_t count_last = RaftTestConfig::rpc_count_last[svr];
  if (reset) {
    RaftTestConfig::rpc_count_last[svr] = count;
  }
  verify(count >= count_last);
  return count - count_last;
}

uint64_t RaftTestConfig::RpcTotal(void) {
  uint64_t total = 0;
  for (int i = 0; i < NSERVERS; i++) {
    total += RaftTestConfig::frames[i]->commo_->rpc_count_;
  }
  return total;
}

bool RaftTestConfig::ServerCommitted(int svr, uint64_t index, int cmd) {
  if (RaftTestConfig::committed_cmds[svr].size() <= index)
    return false;
  return RaftTestConfig::committed_cmds[svr][index] == cmd;
}

void RaftTestConfig::netctlLoop(void) {
  int i;
  bool isdown;
  // cv_m_ unlocked state 0 (finished_ == false)
  Log_debug("netctlLoop: Starting, acquiring cv_m_");
  std::unique_lock<std::mutex> lk(cv_m_);
  Log_debug("netctlLoop: Acquired cv_m_, entering main loop");
  while (!finished_) {
    Log_debug("netctlLoop: Loop iteration, unreliable_=%d, finished_=%d", unreliable_, finished_);
    if (!unreliable_) {
      Log_debug("netctlLoop: Entering cleanup mode (unreliable_=false)");
      {
        Log_debug("netctlLoop: Acquiring disconnect_mtx_ for cleanup");
        std::lock_guard<std::mutex> prlk(disconnect_mtx_);
        Log_debug("netctlLoop: Acquired disconnect_mtx_, starting cleanup loop");
        // unset all unreliable-related disconnects and slows
        for (i = 0; i < NSERVERS; i++) {
          if (!disconnected_[i]) {
            Log_debug("netctlLoop: Cleanup server %d - calling reconnect", i);
            reconnect(i, true);
            Log_debug("netctlLoop: Cleanup server %d - calling slow(0)", i);
            slow(i, 0);
            Log_debug("netctlLoop: Cleanup server %d - done", i);
          }
        }
        Log_debug("netctlLoop: Cleanup loop done, releasing disconnect_mtx_");
      }
      Log_debug("netctlLoop: Cleanup complete, about to wait on cv_");
      // sleep until unreliable_ or finished_ is set
      // cv_m_ unlocked state 1 (unreliable_ == false && finished_ == false)
      cv_.wait(lk, [this](){ return unreliable_ || finished_; });
      Log_debug("netctlLoop: Woke from cv_.wait(), continuing");
      continue;
    }
    Log_debug("netctlLoop: Unreliable mode active, starting random disconnect/reconnect");
    {
      Log_debug("netctlLoop: Acquiring disconnect_mtx_ for unreliable mode");
      std::lock_guard<std::mutex> prlk(disconnect_mtx_);
      Log_debug("netctlLoop: Acquired disconnect_mtx_, processing servers");
      for (i = 0; i < NSERVERS; i++) {
        // skip server if it was disconnected using Disconnect()
        if (disconnected_[i]) {
          continue;
        }
        // server has DOWNRATE_N / DOWNRATE_D chance of being down
        if ((rand() % DOWNRATE_D) < DOWNRATE_N) {
          Log_debug("netctlLoop: Server %d - calling disconnect", i);
          // disconnect server if not already disconnected in the previous period
          disconnect(i, true);
          Log_debug("netctlLoop: Server %d - disconnect done", i);
        } else {
          // Server not down: random slow timeout
          // Reconnect server if it was disconnected in the previous period
          Log_debug("netctlLoop: Server %d - calling reconnect", i);
          reconnect(i, true);
          Log_debug("netctlLoop: Server %d - reconnect done, calling slow", i);
          // server's slow timeout should be btwn 0-(MAXSLOW-1) ms
          slow(i, rand() % MAXSLOW);
          Log_debug("netctlLoop: Server %d - slow done", i);
        }
      }
      Log_debug("netctlLoop: Server processing complete, releasing disconnect_mtx_");
    }
    Log_debug("netctlLoop: About to sleep for 100ms");
    // change unreliable state every 0.1s
    Log_debug("netctlLoop: Sleep complete, releasing cv_m_ briefly");
    // Coroutine::Sleep(100000);
    Log_debug("netctlLoop: cv_m_ released, about to reacquire");
    lk.unlock();
    usleep(100000);
    // cv_m_ unlocked state 2 (unreliable_ == true && finished_ == false)
    lk.lock();
    Log_debug("netctlLoop: cv_m_ reacquired, looping back");
  }
  // If network is still unreliable, unset it
  if (unreliable_) {
    unreliable_ = false;
    {
      std::lock_guard<std::mutex> prlk(disconnect_mtx_);
      // unset all unreliable-related disconnects and slows
      for (i = 0; i < NSERVERS; i++) {
        if (!disconnected_[i]) {
          reconnect(i, true);
          slow(i, 0);
        }
      }
    }
  }
  // cv_m_ unlocked state 3 (unreliable_ == false && finished_ == true)
}

bool RaftTestConfig::isDisconnected(int svr) {
  std::lock_guard<std::recursive_mutex> lk(connection_m_);
  return RaftTestConfig::frames[svr]->svr_->IsDisconnected();
}

void RaftTestConfig::disconnect(int svr, bool ignore) {
  std::lock_guard<std::recursive_mutex> lk(connection_m_);
  if (!isDisconnected(svr)) {
    // simulate disconnected server
    RaftTestConfig::frames[svr]->svr_->Disconnect();
  } else if (!ignore) {
    verify(0);
  }
}

void RaftTestConfig::reconnect(int svr, bool ignore) {
  std::lock_guard<std::recursive_mutex> lk(connection_m_);
  if (isDisconnected(svr)) {
    // simulate reconnected server
    RaftTestConfig::frames[svr]->svr_->Reconnect();
  } else if (!ignore) {
    verify(0);
  }
}

void RaftTestConfig::slow(int svr, uint32_t msec) {
  std::lock_guard<std::recursive_mutex> lk(connection_m_);
  verify(!isDisconnected(svr));
  auto& comm = RaftTestConfig::frames[svr]->commo_;
  verify(comm);
  auto& poll = comm->rpc_poll_;
  verify(poll);
  poll->slow(msec * 1000);
}

RaftServer *RaftTestConfig::GetServer(int svr) {
  return RaftTestConfig::frames[svr]->svr_;
}

#endif

}
