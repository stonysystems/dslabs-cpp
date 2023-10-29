#include "../deptran/__dep__.h"
#include "server.h"
#include "../deptran/raft/server.h"
#include "client.h"

namespace janus {

static int volatile x1 =
    MarshallDeputy::RegInitializer(MarshallDeputy::CMD_MULTI_STRING,
                                     [] () -> Marshallable* {
                                       return new MultiStringMarshallable;
                                     });

KvServer::KvServer(uint64_t maxraftstate=0) {
    maxraftstate_ = maxraftstate;
}

KvServer::~KvServer() {
  /* Your code here for server teardown */

}

int64_t KvServer::GetNextOpId() {
  verify(sp_log_svr_);
  int64_t ret = sp_log_svr_->site_id_;
  ret = ret << 32;
  ret = ret + op_id_cnt_++; 
  return ret;
}

int KvServer::Put(const uint64_t& oid, 
                  const string& k,
                  const string& v) {
  /* 
  Your are recommended to use MultiStringMarshallable as the format 
  for the log entries. Here is an example how you can use it.
  auto s = make_shared<MultiStringMarshallable>();
  s->data_.push_back(to_string(op_id));
  s->data_.push_back("put");
  s->data_.push_back(k);
  s->data_.push_back(v);
  */
  /* your code here */
  return KV_SUCCESS;
}

int KvServer::Append(const uint64_t& oid, 
                     const string& k,
                     const string& v) {
  /* your code here */
  return KV_SUCCESS;
}

int KvServer::Get(const uint64_t& oid, 
                  const string& k,
                  string* v) {
  /* your code here */
  return KV_SUCCESS;
}

void KvServer::OnNextCommand(Marshallable& m) {
  auto v = (MultiStringMarshallable*)(&m);
  /* your code here */
}

shared_ptr<KvClient> KvServer::CreateClient() {
  /* don't change this function */
  auto cli = make_shared<KvClient>();
  cli->commo_ = sp_log_svr_->commo_;
  verify(cli->commo_ != nullptr);
  uint32_t id = sp_log_svr_->site_id_;
  id = id << 16;
  cli->cli_id_ = id+cli_cnt_++; 
  return cli;
}

} // namespace janus;