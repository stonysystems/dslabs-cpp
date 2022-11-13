
#pragma once

#include "../deptran/__dep__.h"
#include "../deptran/raft/server.h"



class ShardConfig {
 public:
  int32_t number{0};
  map<uint32_t, uint32_t> shard_group_map_{{1,0},{2,0},{3,0},{4,0},{5,0},{6,0},{7,0},{8,0},{9,0},{10,0}};
  map<uint32_t, vector<uint32_t>> group_servers_map_{};
  // Lab Shard: you can add functions to this class but do not add/remove member variables
};

inline Marshal& operator>>(Marshal& m, ShardConfig& rhs) {
  m >> rhs.number >> rhs.shard_group_map_ >> rhs.group_servers_map_;
  return m;
}

inline Marshal& operator<<(Marshal& m, const ShardConfig& rhs) {
  m << rhs.number << rhs.shard_group_map_ << rhs.group_servers_map_;
  return m;
}

#include "shardmaster_rpc.h"

namespace janus {

// class TxLogServer;
// class KvServer;
class ShardMasterClient;
class ShardMasterServiceImpl : public ShardMasterService {
 public:
  shared_ptr<TxLogServer> sp_log_svr_{}; 
  const uint64_t SM_TIMEOUT = 10000000; // 10s
  map<uint32_t, ShardConfig> configs_{};   
  // add your own variables here if needed 

  // add your own functions here if needed  

  // do not change anything below
  RaftServer& GetRaftServer() {
    auto p = dynamic_pointer_cast<RaftServer>(sp_log_svr_);
    verify(p != nullptr);
    return *p;
  }
  ShardMasterServiceImpl() {}
  virtual void Join(const std::map<uint32_t, std::vector<uint32_t>>& gid_server_map, uint32_t* ret, rrr::DeferredReply* defer) override;
  virtual void Leave(const std::vector<uint32_t>& gids, uint32_t* ret, rrr::DeferredReply* defer) override;
  virtual void Move(const int32_t& shard, const uint32_t& gid, uint32_t* ret, rrr::DeferredReply* defer) override;
  virtual void Query(const int32_t& config_no, uint32_t* ret, ShardConfig* config, rrr::DeferredReply* defer) override;
  void OnNextCommand(Marshallable& m);
  shared_ptr<ShardMasterClient> CreateClient();
};

} // namespace janus
