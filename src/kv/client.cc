

#include "client.h"
#include "server.h"

namespace janus {

int KvClient::Op(function<int(uint32_t*)> func) {
  uint64_t t1 = Time::now();
  while (true) {
    uint64_t t2 = Time::now();
    if (t2 - t1 > 10000000) {
      return KV_TIMEOUT;
    }
    uint32_t ret = 0;
    int r1; 
    r1 = func(&ret);
    if (r1 == ETIMEDOUT || ret == KV_TIMEOUT) {
      leader_idx_ = (leader_idx_+1) % 5;
      return KV_TIMEOUT;
    }
    if (ret == KV_SUCCESS) {
      return KV_SUCCESS;
    }
    if (ret == KV_NOTLEADER) {
      leader_idx_ = (leader_idx_+1) % 5;
    }
  }

}

int KvClient::Put(const string& k, const string& v) {
  return Op([&](uint32_t* r)->int{
    return Proxy(leader_idx_).Put(GetNextOpId(), k, v, r);
  });
}

KvProxy& KvClient::Proxy(siteid_t site_id) {
  verify(commo_);
  auto p = (KvProxy*)commo_->rpc_proxies_.at(site_id);
  return *p; 
}

int KvClient::Append(const string& k, const string& v) {
  return Op([&](uint32_t* r)->int{
    return Proxy(leader_idx_).Append(GetNextOpId(), k, v, r);
  });
}

int KvClient::Get(const string& k, string* v) {
  return Op([&](uint32_t* r)->int{
    return Proxy(leader_idx_).Get(GetNextOpId(), k, r, v);
  });
}

} // namesapce janus;