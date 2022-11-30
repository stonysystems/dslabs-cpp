#include "../deptran/__dep__.h"
#include "server.h"
#include "../deptran/raft/server.h"
#include "client.h"

namespace janus {

int64_t ShardKvServer::GetNextOpId() {
  verify(sp_log_svr_);
  int64_t ret = sp_log_svr_->site_id_;
  ret = ret << 32;
  ret = ret + op_id_cnt_++; 
  return ret;
}

int ShardKvServer::Put(const uint64_t& oid, 
                  const string& k,
                  const string& v) {
    // lab_shard: fill in your code
}

int ShardKvServer::Append(const uint64_t& oid, 
                     const string& k,
                     const string& v) {
    // lab_shard: fill in your code
}

int ShardKvServer::Get(const uint64_t& oid, 
                  const string& k,
                  string* v) {
    // lab_shard: fill in your code
}

void ShardKvServer::OnNextCommand(Marshallable& m) {
    // lab_shard: fill in your code
}

shared_ptr<ShardKvClient> ShardKvServer::CreateClient(Communicator* comm) {
  auto cli = make_shared<ShardKvClient>();
  cli->commo_ = comm;
  verify(cli->commo_ != nullptr);
  static uint32_t id = 0;
  id++;
  cli->cli_id_ = id; 
  return cli;
}

} // namespace janus;