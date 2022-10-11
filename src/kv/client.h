
#pragma once

#include "../deptran/__dep__.h"
#include "kv_rpc.h"

namespace janus {

class Communicator;
class KvClient {
 public: 
  Communicator* commo_ = nullptr;
  int leader_idx_ = 0; 
  uint32_t cli_id_{UINT32_MAX};
  uint64_t counter_{0};

  KvProxy& Proxy(siteid_t site_id);
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
};

} // namespace janus