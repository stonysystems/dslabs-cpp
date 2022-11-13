
#include "../kv/server.h"
#include "client.h"

namespace janus {

uint32_t ShardMasterClient::Op(function<uint32_t(uint32_t*)> func) {
  uint64_t t1 = Time::now();
  while (true) {
    uint64_t t2 = Time::now();
    if (t2 - t1 > 100000000) {
      return KV_TIMEOUT;
    }
    uint32_t ret = 0;
    int r1; 
    r1 = func(&ret);
    if (r1 == ETIMEDOUT || ret == KV_TIMEOUT) {
      leader_idx_ = (leader_idx_+1) % 5;
      return KV_TIMEOUT;
    }
    if (ret == KV_SUCCESS) {
      return KV_SUCCESS;
    }
    if (ret == KV_NOTLEADER) {
      leader_idx_ = (leader_idx_+1) % 5;
    }
  }
}

ShardMasterProxy& ShardMasterClient::Proxy(siteid_t site_id) {
  verify(commo_);
  auto p = (ShardMasterProxy*)commo_->rpc_proxies_.at(site_id);
  return *p; 
}

uint32_t ShardMasterClient::Join(const std::map<uint32_t, std::vector<uint32_t>>& gid_server_map) {
  return Op([&](uint32_t* r)->uint32_t{
    return Proxy(leader_idx_).Join(gid_server_map, r);
  });
}
uint32_t ShardMasterClient::Leave(const std::vector<uint32_t>& gids) {
  return Op([&](uint32_t* r)->uint32_t{
    return Proxy(leader_idx_).Leave(gids, r);
  });
}
uint32_t ShardMasterClient::Move(const int32_t& shard, const uint32_t& gid) {
  return Op([&](uint32_t* r)->uint32_t{
    return Proxy(leader_idx_).Move(shard, gid, r);
  });
}
uint32_t ShardMasterClient::Query(const int32_t& config_no, ShardConfig* config) {
  return Op([&](uint32_t* r)->uint32_t{
    return Proxy(leader_idx_).Query(config_no, r, config);
  });
}

} // namesapce janus;