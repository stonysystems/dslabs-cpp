
#pragma once

#include "../deptran/__dep__.h"
#include "shardkv_rpc.h"

namespace janus {

class Communicator;
class ShardKvClient {
 public: 
  Communicator* commo_ = nullptr;
  int leader_idx_ = 0; 
  uint32_t cli_id_{UINT32_MAX};
  uint64_t counter_{0};

  ShardKvProxy& Proxy(siteid_t site_id);
  int Op(function<int(uint32_t*)>);
  int Put(const string& k, const string& v);
  int Get(const string& k, string* v);
  int Append(const string& k, const string& v);

  uint64_t GetNextOpId() {
    verify(cli_id_ != UINT32_MAX);
    uint64_t ret = cli_id_;
    ret = ret << 32;
    counter_++;
    ret = ret + counter_; 
    return ret;
  }

  // lab_shard: do not change this function
  shardid_t Key2Shard(string key) {
    shardid_t shard = 0;
    verify(key.length()>0);
    char x = key[0];
    shard = x - '0';
	  shard %= 10; // N shards is 10
	  return shard;
  }

};

} // namespace janus