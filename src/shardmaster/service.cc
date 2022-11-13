
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include "service.h"
#include "client.h"
#include "../kv/server.h"

namespace janus {

void ShardMasterServiceImpl::Join(const map<uint32_t, std::vector<uint32_t>>& gid_server_map, uint32_t* ret, rrr::DeferredReply* defer) {
  // your code here
  defer->reply();
}
void ShardMasterServiceImpl::Leave(const std::vector<uint32_t>& gids, uint32_t* ret, rrr::DeferredReply* defer) {
  // your code here
  defer->reply();
}
void ShardMasterServiceImpl::Move(const int32_t& shard, const uint32_t& gid, uint32_t* ret, rrr::DeferredReply* defer) {
  // your code here
  defer->reply();
}
void ShardMasterServiceImpl::Query(const int32_t& config_no, uint32_t* ret, ShardConfig* config, rrr::DeferredReply* defer) {
  // your code here
  defer->reply();
}

void ShardMasterServiceImpl::OnNextCommand(Marshallable& m) {
  // your code here
}

// do not change anything below
shared_ptr<ShardMasterClient> ShardMasterServiceImpl::CreateClient() {
  auto cli = make_shared<ShardMasterClient>();
  cli->commo_ = sp_log_svr_->commo_;
  uint32_t id = sp_log_svr_->site_id_;
  return cli;
}

} // namespace janus