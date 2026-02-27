
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include "service.h"
#include "client.h"
#include "../kv/server.h"

namespace janus {

void ShardMasterServiceImpl::Join(const map<uint32_t, std::vector<uint32_t>>& gid_server_map, uint32_t* ret, rrr::DeferredReply* defer) {
  auto s = make_shared<MultiStringMarshallable>();
  auto op_id = std::to_string(RandomGenerator::rand(0, INT32_MAX));
  s->data_.push_back(op_id);
  s->data_.push_back("join");
  std::stringstream ss; 
  boost::archive::text_oarchive ar(ss);
  ar << (uint32_t)gid_server_map.size();
  for (auto& pair : gid_server_map) {
    ar << pair.first;
    ar << (uint32_t)pair.second.size();
    for (auto& server_id : pair.second) {
      ar << server_id;
    }
  }
  s->data_.push_back(ss.str());
  uint64_t index;
  uint64_t term;
  auto p = dynamic_pointer_cast<Marshallable>(s);
  if (GetRaftServer().Start(p, &index, &term)) {
    auto ev = Reactor::CreateSpEvent<IntEvent>();
    outstanding_requests_[op_id] = ev;
    ev->Wait(SM_TIMEOUT);
    if (ev->status_ == Event::TIMEOUT) {
      *ret = KV_TIMEOUT;
    }
    *ret = KV_SUCCESS; 
  } else {
    *ret = KV_NOTLEADER;
  }
  defer->reply();
}
void ShardMasterServiceImpl::Leave(const std::vector<uint32_t>& gids, uint32_t* ret, rrr::DeferredReply* defer) {
  auto s = make_shared<MultiStringMarshallable>();
  auto op_id = std::to_string(RandomGenerator::rand(0, INT32_MAX));
  s->data_.push_back(op_id);
  s->data_.push_back("leave");
  std::stringstream ss; 
  boost::archive::text_oarchive ar(ss);
  ar << (uint32_t)gids.size();
  for (auto& gid : gids) {
    ar << gid;
  }
  s->data_.push_back(ss.str());
  uint64_t index;
  uint64_t term;
  auto p = dynamic_pointer_cast<Marshallable>(s);
  if (GetRaftServer().Start(p, &index, &term)) {
    auto ev = Reactor::CreateSpEvent<IntEvent>();
    outstanding_requests_[op_id] = ev;
    ev->Wait(SM_TIMEOUT);
    if (ev->status_ == Event::TIMEOUT) {
      *ret = KV_TIMEOUT;
    }
    *ret = KV_SUCCESS; 
  } else {
    *ret = KV_NOTLEADER;
  }
  defer->reply();
}
void ShardMasterServiceImpl::Move(const int32_t& shard, const uint32_t& gid, uint32_t* ret, rrr::DeferredReply* defer) {
  auto s = make_shared<MultiStringMarshallable>();
  auto op_id = std::to_string(RandomGenerator::rand(0, INT32_MAX));
  s->data_.push_back(op_id);
  s->data_.push_back("leave");
  std::stringstream ss; 
  boost::archive::text_oarchive ar(ss);
  ar << shard << gid;
  s->data_.push_back(ss.str());
  uint64_t index;
  uint64_t term;
  auto p = dynamic_pointer_cast<Marshallable>(s);
  if (GetRaftServer().Start(p, &index, &term)) {
    auto ev = Reactor::CreateSpEvent<IntEvent>();
    outstanding_requests_[op_id] = ev;
    ev->Wait(SM_TIMEOUT);
    if (ev->status_ == Event::TIMEOUT) {
      *ret = KV_TIMEOUT;
    }
    *ret = KV_SUCCESS; 
  } else {
    *ret = KV_NOTLEADER;
  }
  defer->reply();
}
void ShardMasterServiceImpl::Query(const int32_t& config_no, uint32_t* ret, ShardConfig* config, rrr::DeferredReply* defer) {
  auto s = make_shared<MultiStringMarshallable>();
  auto op_id = std::to_string(RandomGenerator::rand(0, INT32_MAX));
  s->data_.push_back(op_id);
  s->data_.push_back("query");
  std::stringstream ss; 
  boost::archive::text_oarchive ar(ss);
  ar << config_no;
  s->data_.push_back(ss.str());
  uint64_t index;
  uint64_t term;
  auto p = dynamic_pointer_cast<Marshallable>(s);
  if (GetRaftServer().Start(p, &index, &term)) {
    auto ev = Reactor::CreateSpEvent<IntEvent>();
    outstanding_requests_[op_id] = ev;
    ev->Wait(SM_TIMEOUT);
    if (ev->status_ == Event::TIMEOUT) {
      *ret = KV_TIMEOUT;
    }
    if (config_no > 0) {
      *config = configs_[config_no];
    } else {
      *config = configs_[latest_config_no_];
    }
    *ret = KV_SUCCESS; 
  } else {
    *ret = KV_NOTLEADER;
  }
  defer->reply();
}

void ShardMasterServiceImpl::OnNextCommand(Marshallable& m) {
  auto v = (MultiStringMarshallable*)(&m);
  verify(v != nullptr);
  verify(v->data_.size()>=3);
  auto& id = v->data_.at(0);
  auto& op_type = v->data_.at(1);
  auto& value = v->data_.at(2);
  stringstream ss(value);
  boost::archive::text_iarchive ar(ss);
  if (op_type == "join") {
    map<uint32_t, std::vector<uint32_t>> gid_server_map;
    uint32_t sz;
    ar >> sz;
    verify(sz < 100);
    for (int i = 0; i < sz; i++) {
      uint32_t gid; 
      ar >> gid;
      auto& v = gid_server_map[gid];
      uint32_t v_sz;
      ar >> v_sz;
      for (int j = 0; j < v_sz; j++) {
        uint32_t sid; 
        ar >> sid;
        v.push_back(sid);
      } 
    }
    auto& current_config = configs_[latest_config_no_];
    latest_config_no_++;
    auto& new_config = configs_[latest_config_no_];
    new_config = current_config;
    new_config.number++; 
    new_config.AddNewReplicaGroups(gid_server_map);
  } else if (op_type == "leave") {
    // do this in the request coroutine.
    auto& value = v->data_.at(2);
    vector<uint32_t> gids;
    uint32_t sz;
    ar >> sz;
    for (int i = 0; i < sz; i++) {
      uint32_t gid; 
      ar >> gid;
      gids.push_back(gid);
    }
    auto& current_config = configs_[latest_config_no_];
    latest_config_no_++;
    auto& new_config = configs_[latest_config_no_];
    new_config = current_config;
    new_config.number++; 
    new_config.RemoveReplicaGroups(gids);
  } else if (op_type == "move") {
    auto& value = v->data_.at(2);
    uint32_t shard;
    uint32_t group;
    ar >> shard >> group;
    auto& current_config = configs_[latest_config_no_];
    latest_config_no_++;
    auto& new_config = configs_[latest_config_no_];
    new_config = current_config;
    new_config.number++; 
    new_config.shard_group_map_[shard] = group;
  } else if (op_type == "query") {
    auto& value = v->data_.at(2);
    int32_t config_no;
    ar >> config_no;
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

shared_ptr<ShardMasterClient> ShardMasterServiceImpl::CreateClient() {
  auto cli = make_shared<ShardMasterClient>();
  cli->commo_ = sp_log_svr_->commo_;
  uint32_t id = sp_log_svr_->site_id_;
  return cli;
}

} // namespace janus