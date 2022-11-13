
#pragma once

#include "../deptran/__dep__.h"
#include "service.h"

namespace janus {

class Communicator;
class ShardMasterClient {
 public: 
  Communicator* commo_ = nullptr;
  int leader_idx_ = 0; 

  ShardMasterProxy& Proxy(siteid_t site_id);
  uint32_t Op(function<uint32_t(uint32_t*)> func);
  uint32_t Join(const std::map<uint32_t, std::vector<uint32_t>>& gid_server_map);
  uint32_t Leave(const std::vector<uint32_t>& gids);
  uint32_t Move(const int32_t& shard, const uint32_t& gid);
  uint32_t Query(const int32_t& config_number, ShardConfig* config);
};
} // namespace janus