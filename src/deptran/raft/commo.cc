
#include "commo.h"
#include "../rcc/graph.h"
#include "../rcc/graph_marshaler.h"
#include "../command.h"
#include "../procedure.h"
#include "../command_marshaler.h"
#include "raft_rpc.h"
#include "macros.h"

namespace janus {

RaftCommo::RaftCommo(PollMgr* poll) : Communicator(poll) {
}

void RaftCommo::SendRequestVote(parid_t par_id,
                                siteid_t site_id,
                                uint64_t arg1,
                                uint64_t arg2) {
  /*
   * Example code for sending a single RPC to server at site_id
   * You may modify and use this function or just use it as a reference
   */
  auto proxies = rpc_par_proxies_[par_id];
  for (auto& p : proxies) {
    if (p.first == site_id) {
      RaftProxy *proxy = (RaftProxy*) p.second;
      FutureAttr fuattr;
      fuattr.callback = [](Future* fu) {
        /* this is a handler that will be invoked when the RPC returns */
        uint64_t ret1;
        bool_t vote_granted;
        /* retrieve RPC return values in order */
        fu->get_reply() >> ret1;
        fu->get_reply() >> vote_granted;
        /* process the RPC response here */
      };
      /* Always use Call_Async(proxy, RPC name, RPC args..., fuattr)
      * to asynchronously invoke RPCs */
      Call_Async(proxy, RequestVote, arg1, arg2, fuattr);
    }
  }
}

void RaftCommo::SendAppendEntries(parid_t par_id,
                                  siteid_t site_id,
                                  shared_ptr<Marshallable> cmd) {
  /*
   * More example code for sending a single RPC to server at site_id
   * You may modify and use this function or just use it as a reference
   */
  auto proxies = rpc_par_proxies_[par_id];
  for (auto& p : proxies) {
    if (p.first == site_id) {
      RaftProxy *proxy = (RaftProxy*) p.second;
      FutureAttr fuattr;
      fuattr.callback = [](Future* fu) {
        bool_t followerAppendOK;
        fu->get_reply() >> followerAppendOK;
      };
      /* wrap Marshallable in a MarshallDeputy to send over RPC */
      MarshallDeputy md(cmd);
      Call_Async(proxy, AppendEntries, md, fuattr);
    }
  }
}

shared_ptr<IntEvent> 
RaftCommo::SendString(parid_t par_id, siteid_t site_id, const string& msg, string* res) {
  auto proxies = rpc_par_proxies_[par_id];
  auto ev = Reactor::CreateSpEvent<IntEvent>();
  for (auto& p : proxies) {
    if (p.first == site_id) {
      RaftProxy *proxy = (RaftProxy*) p.second;
      FutureAttr fuattr;
      fuattr.callback = [res,ev](Future* fu) {
        fu->get_reply() >> *res;
        ev->Set(1);
      };
      /* wrap Marshallable in a MarshallDeputy to send over RPC */
      Call_Async(proxy, HelloRpc, msg, fuattr);
    }
  }
  return ev;
}


} // namespace janus
