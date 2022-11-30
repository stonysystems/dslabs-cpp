
#pragma once

#include "../deptran/__dep__.h"
#include "../deptran/marshallable.h"
#include "../deptran/communicator.h"
#include "../deptran/raft/server.h"
#include "../kv/server.h"

namespace janus {
class RaftServer;

class ShardKvClient;
class ShardKvServer {
 public:

  shared_ptr<TxLogServer> sp_log_svr_; 
  int64_t op_id_cnt_ = 0;
  uint32_t cli_cnt_ = 0;
  map<string, shared_ptr<IntEvent>> outstanding_requests_{};
  map<string, string> kv_store_;

  RaftServer& GetRaftServer() {
    auto p = dynamic_pointer_cast<RaftServer>(sp_log_svr_);
    verify(p != nullptr);
    return *p;
  }

  int64_t GetNextOpId();

  int Put(const uint64_t& op_id,
          const string& k,
          const string& v);

  int Append(const uint64_t& op_id, 
             const string& k,
             const string& v);

  int Get(const uint64_t& op_id, 
          const string& k,
          string* v);

  void OnNextCommand(Marshallable& m);

  static shared_ptr<ShardKvClient> CreateClient(Communicator* comm);
};

}; // namespace janus