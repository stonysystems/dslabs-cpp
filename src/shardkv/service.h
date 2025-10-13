
#pragma once

#include "../deptran/__dep__.h"
// #include "../deptran/constants.h"
// #include "../deptran/command.h"
// #include "../deptran/procedure.h"
// #include "../deptran/command_marshaler.h"
#include "shardkv_rpc.h"

namespace janus {

class TxLogServer;
class ShardKvServer;
class ShardKvServiceImpl : public ShardKvService {
 public:
  shared_ptr<ShardKvServer> sp_svr_{nullptr};
  ShardKvServiceImpl() {}
   
  void Put(const uint64_t& op_id, 
           const string& k,
           const string& v,
           uint32_t* ret,
           rrr::DeferredReply* defer) override;

  void Append(const uint64_t& op_id, 
              const string& k,
              const string& v,
              uint32_t* ret,
              rrr::DeferredReply* defer) override;

  void Get(const uint64_t& op_id, 
           const string& k,
           uint32_t* ret,
           string* v,
           rrr::DeferredReply* defer) override;

};

} // namespace janus
