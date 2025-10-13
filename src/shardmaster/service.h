
#pragma once

#include "../deptran/__dep__.h"
#include "../deptran/raft/server.h"



class ShardConfig {
 public:
  int32_t number{0};
  map<uint32_t, uint32_t> shard_group_map_{{1,0},{2,0},{3,0},{4,0},{5,0},{6,0},{7,0},{8,0},{9,0},{10,0}};
  map<uint32_t, vector<uint32_t>> group_servers_map_{};

  void Rebalance() {
    verify(group_servers_map_.size() > 0);
    verify(group_servers_map_.find(0) == group_servers_map_.end());
    uint32_t n_shard_per_group = (shard_group_map_.size() + group_servers_map_.size() - 1) / group_servers_map_.size();  
    map<uint32_t, vector<uint32_t>> group_shards_map{};
    // build group/shards mapping 
    for (auto& pair : shard_group_map_) {
      auto shard_id = pair.first;
      auto group_id = pair.second;
      auto& v = group_shards_map[group_id];
      v.push_back(shard_id);
    }
    // find out who has more and who has less
    map<uint32_t, vector<uint32_t>> group_shards_more{};
    map<uint32_t, vector<uint32_t>> group_shards_less{};
    for (auto& pair: group_servers_map_) {
      auto group = pair.first;
      auto& shards = group_shards_map[group];
      if (shards.size() > n_shard_per_group) {
        group_shards_more[group] = shards;   
      } else if (shards.size() < n_shard_per_group) {
        group_shards_less[group] = shards;   
      }
    }
    auto& shardsofgroup0 = group_shards_map[0];
    if (shardsofgroup0.size() > 0) {
      group_shards_more[0] = shardsofgroup0;
    }
    while (group_shards_more.size() > 0 && group_shards_less.size() > 0) {
      // find more 
      uint32_t shard = 0;
      for (auto it = group_shards_more.begin(); it != group_shards_more.end();) {
        auto group = it->first;
        auto& shards = it->second; 
        if ((group==0 && shards.size() > 0) || shards.size() > n_shard_per_group) {
          shard = shards.back();
          shards.pop_back();
          break;
        } else {
          it = group_shards_more.erase(it);
        } 
      }
      for (auto it = group_shards_less.begin(); it != group_shards_less.end();) {
        auto group_id = it->first;
        auto& shards = it->second; 
        verify (shards.size() < n_shard_per_group);
        shards.push_back(shard);
        shard_group_map_[shard] = group_id;   
        if (n_shard_per_group == shards.size()) {
          group_shards_less.erase(it);   
        }
        break;
      }
    }
    verify(group_servers_map_.find(0) == group_servers_map_.end());
    int cnt = 0;
    for (auto& pair : shard_group_map_) {
      if (pair.second == 0) {
        cnt++;
      }
    }
    if (cnt > 0) {
      verify(group_servers_map_.size() == 0);
    }
  }

  void AddNewReplicaGroups(map<uint32_t, vector<uint32_t>>& gid_svr_map) {
    group_servers_map_.insert(gid_svr_map.begin(), gid_svr_map.end()); 
    Rebalance();
  }

  void RemoveReplicaGroups(vector<uint32_t>& gids) {
    uint32_t target_group = 0;
    for (auto& pair : group_servers_map_) {
      auto& gid = pair.first;
      if (std::none_of(gids.begin(), gids.end(), [&](uint32_t g) {return gid == g;})) {
        target_group = gid;
        break;
      }
    }
    verify(target_group != 0);
    for (auto g : gids) {
      for (auto& pair : shard_group_map_) {
        auto& shard = pair.first;
        auto& group = pair.second;
        if (g == group) {
          group = target_group;
        }
      }           
      group_servers_map_.erase(g);
    }
    Rebalance();
  }
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
  uint32_t latest_config_no_{0};
  map<string, shared_ptr<IntEvent>> outstanding_requests_{};
  
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
