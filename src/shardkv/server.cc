#include "../protocol/__dep__.h"
#include "server.h"
#include "../protocol/raft/server.h"
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
  auto s = make_shared<MultiStringMarshallable>();
  auto op_id = std::to_string(oid);
  s->data_.push_back(op_id);
  s->data_.push_back("put");
  s->data_.push_back(k);
  s->data_.push_back(v);
  uint64_t index;
  uint64_t term;
  auto p = dynamic_pointer_cast<Marshallable>(s);
  if (GetRaftServer().Start(p, &index, &term)) {
    auto ev = Reactor::CreateSpEvent<IntEvent>();
    outstanding_requests_[op_id] = ev;
    ev->Wait(1000000);
    if (ev->status_ == Event::TIMEOUT) {
      return KV_TIMEOUT;
    }
    return KV_SUCCESS; 
  } else {
    return KV_NOTLEADER;
  }
}

int ShardKvServer::Append(const uint64_t& oid, 
                     const string& k,
                     const string& v) {

  auto s = make_shared<MultiStringMarshallable>();
  auto op_id = std::to_string(oid);
  s->data_.push_back(op_id);
  s->data_.push_back("append");
  s->data_.push_back(k);
  s->data_.push_back(v);
  verify(s->data_[1] == "append");
  uint64_t index;
  uint64_t term;
  auto p = dynamic_pointer_cast<Marshallable>(s);
  if (GetRaftServer().Start(p, &index, &term)) {
    auto ev = Reactor::CreateSpEvent<IntEvent>();
    outstanding_requests_[op_id] = ev;
    ev->Wait(1000000);
    if (ev->status_ == Event::TIMEOUT) {
      return KV_TIMEOUT;
    }
    return KV_SUCCESS; 
  } else {
    return KV_NOTLEADER;
  }
}

int ShardKvServer::Get(const uint64_t& oid, 
                  const string& k,
                  string* v) {
  auto s = make_shared<MultiStringMarshallable>();
  auto op_id = std::to_string(oid);
  s->data_.push_back(op_id);
  s->data_.push_back("get");
  s->data_.push_back(k);
  uint64_t index;
  uint64_t term;
  auto p = dynamic_pointer_cast<Marshallable>(s);
  GetRaftServer().Start(p, &index, &term); //TODO 
  auto ev = Reactor::CreateSpEvent<IntEvent>();
  outstanding_requests_[op_id] = ev;
  ev->Wait(1000000);
  if (ev->status_ == Event::TIMEOUT) {
    return KV_TIMEOUT;
  }
  *v = kv_store_[k];
  return KV_SUCCESS; 
}

void ShardKvServer::OnNextCommand(Marshallable& m) {
  auto v = (MultiStringMarshallable*)(&m);
  verify(v != nullptr);
  verify(v->data_.size()>=3);
  auto& id = v->data_.at(0);
  auto& op_type = v->data_.at(1);
  auto& key = v->data_.at(2);
  if (op_type == "put") {
    auto& value = v->data_.at(3);
    kv_store_[key] = value;
  } else if (op_type == "get") {
    // do this in the request coroutine.
  } else if (op_type == "append") {
    auto& value = v->data_.at(3);
    kv_store_[key].append(value);
  } else {
    verify(0);
  }
  auto it = outstanding_requests_.find(id);
  if (it != outstanding_requests_.end()) {
    auto ev = it->second;
    outstanding_requests_.erase(it);
    ev->Set(1);
  } 
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