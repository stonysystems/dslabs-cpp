
#pragma once

#include "../protocol/__dep__.h"
// #include "../protocol/constants.h"
// #include "../protocol/command.h"
// #include "../protocol/procedure.h"
// #include "../protocol/command_marshaler.h"
#include "kv_rpc.h"

namespace janus {

class TxLogServer;
class KvServer;
class KvServiceImpl : public KvService {
 public:
  shared_ptr<KvServer> sp_svr_{nullptr};
  KvServiceImpl() {}
   
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
