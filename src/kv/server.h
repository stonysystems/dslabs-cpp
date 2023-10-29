
#pragma once

#include "../deptran/__dep__.h"
#include "../deptran/marshallable.h"
#include "../deptran/communicator.h"
#include "../deptran/raft/server.h"

namespace janus {
 
class RaftServer;

const int KV_SUCCESS = 0;
const int KV_TIMEOUT = 1;
const int KV_NOTLEADER = 2;

class MultiStringMarshallable : public Marshallable {
 public:
  MultiStringMarshallable() : Marshallable(MarshallDeputy::CMD_MULTI_STRING) {} 
  vector<string> data_{};
  Marshal& ToMarshal(Marshal& m) const override {
    int32_t sz = data_.size();
    m << sz;
    for (auto& str : data_) {
      m << str;
    }
    return m;
  }

  Marshal& FromMarshal(Marshal& m) override {
    int32_t sz;
    m >> sz;
    data_.resize(sz, "");
    for (auto& str : data_) {
      string s;
      m >> s;
      Log_debug(s.c_str());
      str = s;
    }
    return m;
  }
};

class KvClient;
class KvServer {
 public:

  shared_ptr<TxLogServer> sp_log_svr_; 
  int64_t op_id_cnt_ = 0;
  uint32_t cli_cnt_ = 0;
  uint64_t maxraftstate_ = 0;

  KvServer(uint64_t maxraftstate) ;
  ~KvServer() ;
  /* add your variables here */

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

  shared_ptr<KvClient> CreateClient();
};

}; // namespace janus