#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../scheduler.h"
#include "../classic/tpc_command.h"
#include "commo.h"
#include "persister.h"

namespace janus {

#define HEARTBEAT_INTERVAL 100000

class RaftServer : public TxLogServer {
 public:
   shared_ptr<Persister> persister;
  /* Your data here */

  /* Your functions here */

  /* do not modify this class below here */

 public:
  RaftServer(Frame *frame, shared_ptr<Persister> persister) ;
  ~RaftServer() ;

  bool Start(shared_ptr<Marshallable> &cmd, uint64_t *index, uint64_t *term);
  void GetState(bool *is_leader, uint64_t *term);
  void Persist();
  void ReadPersist();

 private:
  bool disconnected_ = false;
  void Setup();
  void Shutdown();

 public:
  void SyncRpcExample();
  void Disconnect(const bool disconnect = true);
  void Reconnect() {
    Disconnect(false);
  }
  bool IsDisconnected();

  virtual bool HandleConflicts(Tx& dtxn,
                               innid_t inn_id,
                               vector<string>& conflicts) {
    verify(0);
  };
  RaftCommo* commo() {
    return (RaftCommo*)commo_;
  }
};
} // namespace janus
