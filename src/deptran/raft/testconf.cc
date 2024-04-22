#include "testconf.h"
#include "marshallable.h"

namespace janus {

#ifdef RAFT_TEST_CORO

int _test_id_g = 0;

RaftFrame **RaftTestConfig::replicas = nullptr;
std::function<void(Marshallable &)> RaftTestConfig::commit_callbacks[NSERVERS];
std::vector<int> RaftTestConfig::committed_cmds[NSERVERS];
uint64_t RaftTestConfig::rpc_count_last[NSERVERS];

RaftTestConfig::RaftTestConfig(RaftFrame **replicas) {
  verify(RaftTestConfig::replicas == nullptr);
  RaftTestConfig::replicas = replicas;
  for (int i = 0; i < NSERVERS; i++) {
    RaftTestConfig::replicas[i]->svr_->rep_frame_ = RaftTestConfig::replicas[i]->svr_->frame_;
    RaftTestConfig::committed_cmds[i].push_back(-1);
    RaftTestConfig::rpc_count_last[i] = 0;
    disconnected_[i] = false;
  }
  th_ = std::thread([this](){ netctlLoop(); });
}

void RaftTestConfig::SetLearnerAction(void) {
  for (int i = 0; i < NSERVERS; i++) {
    RaftTestConfig::commit_callbacks[i] = [i](Marshallable& cmd) {
      verify(cmd.kind_ == MarshallDeputy::CMD_TPC_COMMIT);
      auto& command = dynamic_cast<TpcCommitCommand&>(cmd);
      Log_debug("server %d committed value %d", i, command.tx_id_);
      RaftTestConfig::committed_cmds[i].push_back(command.tx_id_);
    };
    RaftTestConfig::replicas[i]->svr_->RegLearnerAction(RaftTestConfig::commit_callbacks[i]);
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
  for (retry = 0; retry < 10; retry++) {
    // Reactor::CreateSpEvent<TimeoutEvent>(ELECTIONTIMEOUT / 10)->Wait();
    // Coroutine::Sleep(ELECTIONTIMEOUT/10);
    usleep(ELECTIONTIMEOUT/10);
    leader = -1;
    mostRecentTerm = 0;
    for (i = 0; i < NSERVERS; i++) {
      // ignore disconnected servers
      if (RaftTestConfig::replicas[i]->svr_->IsDisconnected())
        continue;
      RaftTestConfig::replicas[i]->svr_->GetState(&isleader, &term);
      if (isleader) {
        if (term == mostRecentTerm) {
          Failed("multiple leaders elected in term %ld", term);
          return -2;
        } else if (term > mostRecentTerm) {
          leader = i;
          mostRecentTerm = term;
          Log_debug("found leader %d with term %d", leader, term);
        }
      }
    }
    if (leader != -1) {
      if (!want_leader) {
        Failed("leader elected despite lack of quorum");
      } else if (expected >= 0 && leader != expected) {
        Failed("unexpected leader change, expecting %d, got %d", expected, leader);
        return -3;
      }
      return leader;
    }
  }
  if (want_leader) {
    Log_debug("failing, timeout?");
    Failed("waited too long for leader election");
  }
  return -1;
}

bool RaftTestConfig::TermMovedOn(uint64_t term) {
  for (int i = 0; i < NSERVERS; i++) {
    uint64_t curTerm;
    bool isLeader;
    RaftTestConfig::replicas[i]->svr_->GetState(&isLeader, &curTerm);
    if (curTerm > term) {
      return true;
    }
  }
  return false;
}

uint64_t RaftTestConfig::OneTerm(void) {
  uint64_t term, curTerm;
  bool isLeader;
  RaftTestConfig::replicas[0]->svr_->GetState(&isLeader, &term);
  for (int i = 1; i < NSERVERS; i++) {
    RaftTestConfig::replicas[i]->svr_->GetState(&isLeader, &curTerm);
    if (curTerm != term) {
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
  return RaftTestConfig::replicas[svr]->svr_->Start(cmdptr_m, index, term);
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
    Reactor::CreateSpEvent<TimeoutEvent>(to)->Wait();
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
      if (RaftTestConfig::replicas[i]->svr_->IsDisconnected())
        continue;
      if (Start(i, cmd, &index, &term)) {
        Log_debug("starting cmd ldr=%d cmd=%d index=%ld term=%ld", 
            RaftTestConfig::replicas[i]->svr_->loc_id_, cmd, index, term);
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
  std::unique_lock<std::mutex> lk(cv_m_);
  verify(!finished_);
  if (unreliable) {
    verify(!unreliable_);
    // lk acquired cv_m_ in state 1 or 0
    unreliable_ = true;
    // if cv_m_ was in state 1, must signal cv_ to wake up netctlLoop
    lk.unlock();
    cv_.notify_one();
  } else {
    verify(unreliable_);
    // lk acquired cv_m_ in state 2 or 0
    unreliable_ = false;
    // wait until netctlLoop moves cv_m_ from state 2 (or 0) to state 1,
    // restoring the network to reliable state in the process.
    lk.unlock();
    lk.lock();
  }
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
    RaftTestConfig::replicas[svr]->commo_->rpc_mtx_);
  uint64_t count = RaftTestConfig::replicas[svr]->commo_->rpc_count_;
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
    total += RaftTestConfig::replicas[i]->commo_->rpc_count_;
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
  std::unique_lock<std::mutex> lk(cv_m_);
  while (!finished_) {
    if (!unreliable_) {
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
      // sleep until unreliable_ or finished_ is set
      // cv_m_ unlocked state 1 (unreliable_ == false && finished_ == false)
      cv_.wait(lk, [this](){ return unreliable_ || finished_; });
      continue;
    }
    {
      std::lock_guard<std::mutex> prlk(disconnect_mtx_);
      for (i = 0; i < NSERVERS; i++) {
        // skip server if it was disconnected using Disconnect()
        if (disconnected_[i]) {
          continue;
        }
        // server has DOWNRATE_N / DOWNRATE_D chance of being down
        if ((rand() % DOWNRATE_D) < DOWNRATE_N) {
          // disconnect server if not already disconnected in the previous period
          disconnect(i, true);
        } else {
          // Server not down: random slow timeout
          // Reconnect server if it was disconnected in the previous period
          reconnect(i, true);
          // server's slow timeout should be btwn 0-(MAXSLOW-1) ms
          slow(i, rand() % MAXSLOW);
        }
      }
    }
    // change unreliable state every 0.1s
    usleep(100000);
    // Coroutine::Sleep(100000);
    lk.unlock();
    // cv_m_ unlocked state 2 (unreliable_ == true && finished_ == false)
    lk.lock();
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
  return RaftTestConfig::replicas[svr]->svr_->IsDisconnected();
}

void RaftTestConfig::disconnect(int svr, bool ignore) {
  std::lock_guard<std::recursive_mutex> lk(connection_m_);
  if (!isDisconnected(svr)) {
    // simulate disconnected server
    RaftTestConfig::replicas[svr]->svr_->Disconnect();
  } else if (!ignore) {
    verify(0);
  }
}

void RaftTestConfig::reconnect(int svr, bool ignore) {
  std::lock_guard<std::recursive_mutex> lk(connection_m_);
  if (isDisconnected(svr)) {
    // simulate reconnected server
    RaftTestConfig::replicas[svr]->svr_->Reconnect();
  } else if (!ignore) {
    verify(0);
  }
}

void RaftTestConfig::slow(int svr, uint32_t msec) {
  std::lock_guard<std::recursive_mutex> lk(connection_m_);
  verify(!isDisconnected(svr));
  // Ref<PollMgr> cpoll = borrow_const(RaftTestConfig::replicas[svr]->commo_->rpc_poll_);
  RaftTestConfig::replicas[svr]->commo_->rpc_poll_->slow(msec * 1000);
}

RaftServer *RaftTestConfig::GetServer(int svr) {
  return RaftTestConfig::replicas[svr]->svr_;
}

#endif

}
