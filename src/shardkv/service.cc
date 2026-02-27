#include "service.h"
#include "server.h"

namespace janus {

class TxLogServer;
class RaftServer;

void ShardKvServiceImpl::Put(const uint64_t& opid,
                             const string& k,
                             const string& v,
                             uint32_t* ret,
                             rrr::DeferredReply* defer) {
  *ret = sp_svr_->Put(opid, k, v);
  defer->reply();
}

void ShardKvServiceImpl::Append(const uint64_t& opid, 
                                const string& k,
                                const string& v,
                                uint32_t* ret,
                                rrr::DeferredReply* defer) {
  *ret = sp_svr_->Append(opid, k, v);
  defer->reply();
}

void ShardKvServiceImpl::Get(const uint64_t& opid, 
                             const string& k,
                             uint32_t* ret,
                             string* v,
                             rrr::DeferredReply* defer) {
  *ret = sp_svr_->Get(opid, k, v);
  defer->reply();
}

} // namespace janus