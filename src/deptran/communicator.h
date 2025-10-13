#pragma once

#include "__dep__.h"
#include "constants.h"
#include "msg.h"
#include "config.h"
#include "command_marshaler.h"
#include "deptran/rcc/dep_graph.h"
#include "rcc_rpc.h"
#include <ctime>
#include <rusty/arc.hpp>

namespace janus {

static void _wan_wait() {
  int num = 50;
  Reactor::CreateSpEvent<NeverEvent>()->Wait(num*1000);
}

static void _wan_wait_time(int m) {
  this_thread::sleep_for(chrono::milliseconds(m));
}


#ifdef SIMULATE_WAN

#define WAN_WAIT _wan_wait();

#define WAN_WAIT_TIME(m) _wan_wait_time(m);

#else

#define WAN_WAIT ;
#define WAN_WAIT_TIME ;

#endif

class Coordinator;
class ClassicProxy;
class ClientControlProxy;

typedef std::pair<siteid_t, ClassicProxy*> SiteProxyPair;
typedef std::pair<siteid_t, ClientControlProxy*> ClientSiteProxyPair;

class MessageEvent : public IntEvent {
 public:
  shardid_t shard_id_;
  svrid_t svr_id_;
  string msg_;
  MessageEvent(svrid_t svr_id) : IntEvent(), svr_id_(svr_id) {

  }

  MessageEvent(shardid_t shard_id, svrid_t svr_id)
      : IntEvent(), shard_id_(shard_id), svr_id_(svr_id) {

  }
};

class PaxosPrepareQuorumEvent: public QuorumEvent {
 public:
  using QuorumEvent::QuorumEvent;
//  ballot_t max_ballot_{0};
  bool HasAcceptedValue() {
    // TODO implement this
    return false;
  }
  void FeedResponse(bool y) {
    if (y) {
      n_voted_yes_++;
    } else {
      n_voted_no_++;
    }
  }


};

class PaxosAcceptQuorumEvent: public QuorumEvent {
 public:
  using QuorumEvent::QuorumEvent;
  void FeedResponse(bool y) {
    if (y) {
      n_voted_yes_++;
    } else {
      n_voted_no_++;
    }
  }
};

class GetLeaderQuorumEvent : public QuorumEvent {
 public:
  using QuorumEvent::QuorumEvent;
  void FeedResponse(bool y, locid_t leader_id) {
    if (y) {
      leader_id_ = leader_id;
      VoteYes();
    } else {
      VoteNo();
    }
  }

  bool No() override { return n_voted_no_ == n_total_; }

  bool IsReady() override {
    if (Yes()) {
      return true;
    } else if (No()) {
      return true;
    }
    return false;
  }
};


class Communicator {
 public:
  const int CONNECT_TIMEOUT_MS = 120*1000;
  const int CONNECT_SLEEP_MS = 1000;
  rusty::Arc<rrr::PollThreadWorker> rpc_poll_;
  locid_t loc_id_ = -1;
  TxLogServer *rep_sched_ = nullptr;  // Bidirectional link to replication scheduler
  map<siteid_t, std::shared_ptr<rrr::Client>> rpc_clients_{};
  map<siteid_t, ClassicProxy *> rpc_proxies_{};
  map<parid_t, vector<SiteProxyPair>> rpc_par_proxies_{};
  map<parid_t, SiteProxyPair> leader_cache_ = {};
  vector<ClientSiteProxyPair> client_leaders_;
  std::atomic_bool client_leaders_connected_;
  std::vector<std::thread> threads;

  // Lab-solution fields for re-election and performance monitoring
  static uint64_t global_id;
  unordered_map<uint64_t, pair<rrr::i64, rrr::i64>> outbound_{};
  map<uint64_t, double> lat_util_{};
  locid_t leader_ = 0;
  int outbound = 0;
  int outbounds[100];
  int ob_index = 0;
  int begin_index = 0;
  bool paused = false;
  bool slow = false;
  int index = 0;
  int cpu_index = 0;
  int low_util = 0;
  int total = 0;
  int total_ = 0;
  shared_ptr<QuorumEvent> qe;
  rrr::i64 window[200];
  rrr::i64 window_time = 0;
  rrr::i64 total_time = 0;
  rrr::i64 window_avg = 0;
  rrr::i64 total_avg = 0;
  double cpu_stor[10];
  double cpu_total = 0.0;
  double cpu = 1.0;
  double last_cpu = 1.0;
  double tx = 0.0;
  bool follower_forwarding = false;
  std::mutex lock_;
  std::mutex count_lock_;
  std::condition_variable cv_;
  bool waiting = false;

  Communicator(rusty::Arc<PollThreadWorker> poll_mgr = rusty::Arc<PollThreadWorker>());
  virtual ~Communicator();

  SiteProxyPair RandomProxyForPartition(parid_t partition_id) const;
  SiteProxyPair LeaderProxyForPartition(parid_t) const;
  SiteProxyPair NearestProxyForPartition(parid_t) const;
  virtual SiteProxyPair DispatchProxyForPartition(parid_t par_id) const {
    return LeaderProxyForPartition(par_id);
  };
  std::pair<int, ClassicProxy*> ConnectToSite(Config::SiteInfo &site,
                                              std::chrono::milliseconds timeout_ms);
  ClientSiteProxyPair ConnectToClientSite(Config::SiteInfo &site,
                                          std::chrono::milliseconds timeout);
  void ConnectClientLeaders();
  void WaitConnectClientLeaders();

  vector<function<bool(const string& arg, string& ret)> >
      msg_string_handlers_{};
  vector<function<bool(const MarshallDeputy& arg,
                       MarshallDeputy& ret)> > msg_marshall_handlers_{};

  void SendStart(SimpleCommand& cmd,
                 int32_t output_size,
                 std::function<void(Future *fu)> &callback);
  void BroadcastDispatch(shared_ptr<vector<shared_ptr<SimpleCommand>>> vec_piece_data,
                         Coordinator *coo,
                         const std::function<void(int res, TxnOutput &)> &) ;
  shared_ptr<IntEvent> BroadcastDispatch(ReadyPiecesData cmds_by_par,
                                          Coordinator* coo,
                                          TxData* txn);
  void SendPrepare(parid_t gid,
                   txnid_t tid,
                   std::vector<int32_t> &sids,
                   const std::function<void(int)> &callback) ;
  void SendCommit(parid_t pid,
                  txnid_t tid,
                  const std::function<void()> &callback) ;
  void SendAbort(parid_t pid,
                 txnid_t tid,
                 const std::function<void()> &callback) ;

  // for debug
  std::set<std::pair<parid_t, txnid_t>> phase_three_sent_;

  void ___LogSent(parid_t pid, txnid_t tid);

  void SendUpgradeEpoch(epoch_t curr_epoch,
                        const function<void(parid_t,
                                            siteid_t,
                                            int32_t& graph)>& callback);

  void SendTruncateEpoch(epoch_t old_epoch);
  void SendForwardTxnRequest(TxRequest& req, Coordinator* coo, std::function<void(const TxReply&)> callback);

  /**
   *
   * @param shard_id 0 means broadcast to all shards.
   * @param svr_id 0 means broadcast to all replicas in that shard.
   * @param msg
   */
  vector<shared_ptr<MessageEvent>> BroadcastMessage(shardid_t shard_id,
                                                    svrid_t svr_id,
                                                    string& msg);
  std::shared_ptr<MessageEvent> SendMessage(svrid_t svr_id, string& msg);

  void AddMessageHandler(std::function<bool(const string&, string&)>);
  void AddMessageHandler(std::function<bool(const MarshallDeputy&,
                                            MarshallDeputy&)>);

  // Lab-solution methods for re-election and coordinator-based RPCs
  void ResetProfiles();
  shared_ptr<QuorumEvent> SendReelect();
  shared_ptr<AndEvent> SendPrepare(Coordinator* coo,
                                   txnid_t tid,
                                   std::vector<int32_t>& sids);
  shared_ptr<AndEvent> SendCommit(Coordinator* coo,
                                  txnid_t tid);
  shared_ptr<AndEvent> SendAbort(Coordinator* coo,
                                 txnid_t tid);
  void SendEarlyAbort(parid_t pid, txnid_t tid);
  shared_ptr<GetLeaderQuorumEvent> BroadcastGetLeader(parid_t par_id, locid_t cur_pause);
  shared_ptr<QuorumEvent> SendFailOverTrig(parid_t par_id, locid_t loc_id, bool pause);
  void SetNewLeaderProxy(parid_t par_id, locid_t loc_id);

  virtual shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastBulkPrepare(parid_t par_id,
                        shared_ptr<Marshallable> cmd,
                        std::function<void(ballot_t, int)> cb){
      verify(0);
    }

  virtual shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastHeartBeat(parid_t par_id,
                        shared_ptr<Marshallable> cmd,
                        const std::function<void(ballot_t, int)>& cb){
      verify(0);
    }

    virtual void ForwardToLearner(parid_t par_id,
                                  uint64_t slot,
                                  ballot_t ballot,
                                  shared_ptr<Marshallable> cmd,
                                  const std::function<void(uint64_t, ballot_t)>& cb) {
      verify(0);
    }

  virtual shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastSyncLog(parid_t par_id,
                      shared_ptr<Marshallable> cmd,
                      const std::function<void(shared_ptr<MarshallDeputy>, ballot_t, int)>& cb){
      verify(0);
    }

   virtual shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastSyncNoOps(parid_t par_id,
                    shared_ptr<Marshallable> cmd,
                    const std::function<void(ballot_t, int)>& cb){

	verify(0);
   }

  virtual shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastSyncCommit(parid_t par_id,
                      shared_ptr<Marshallable> cmd,
                      const std::function<void(ballot_t, int)>& cb){
      verify(0);
    }
};

} // namespace janus
