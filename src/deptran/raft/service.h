#pragma once

#include "__dep__.h"
#include "constants.h"
#include "../rcc/graph.h"
#include "../rcc/graph_marshaler.h"
#include "../command.h"
#include "deptran/procedure.h"
#include "../command_marshaler.h"
#include "raft_rpc.h"
#include "server.h"
#include "macros.h"

class SimpleCommand;
namespace janus {

class TxLogServer;
class RaftServer;
class RaftServiceImpl : public RaftService {
 public:
  RaftServer* svr_;
  RaftServiceImpl(TxLogServer* sched);

  RpcHandler(RequestVote, 4,
             const uint64_t&, arg1,
             const uint64_t&, arg2,
             uint64_t*, ret1,
             bool_t*, vote_granted) {
    *ret1 = 0;
    *vote_granted = false;
  }

  RpcHandler(AppendEntries, 2,
             const MarshallDeputy&, cmd,
             bool_t*, followerAppendOK) {
    *followerAppendOK = false;
  }

  RpcHandler(HelloRpc, 2, const string&, req, string*, res) {
    *res = "error"; 
  };

};

} // namespace janus
