#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../communicator.h"

namespace janus {

class TxData;

class RaftCommo : public Communicator {

 public:
  RaftCommo() = delete;
  RaftCommo(PollMgr*);

  void SendRequestVote(parid_t par_id,
                       siteid_t site_id,
                       uint64_t arg1,
                       uint64_t arg2);

  void SendAppendEntries(parid_t par_id,
                         siteid_t site_id,
                         shared_ptr<Marshallable> cmd);

  shared_ptr<IntEvent> 
  SendString(parid_t par_id, siteid_t site_id, const string& msg, string* res);

  /* Do not modify this class below here */

  friend class FpgaRaftProxy;
 public:
#ifdef RAFT_TEST_CORO
  std::recursive_mutex rpc_mtx_ = {};
  uint64_t rpc_count_ = 0;
#endif
};

} // namespace janus

