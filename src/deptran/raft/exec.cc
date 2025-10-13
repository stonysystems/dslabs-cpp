#include "exec.h"

namespace janus {


ballot_t RaftExecutor::Prepare(const ballot_t ballot) {
  verify(0);
  return 0;
}

ballot_t RaftExecutor::Accept(const ballot_t ballot,
                                    shared_ptr<Marshallable> cmd) {
  verify(0);
  return 0;
}

ballot_t RaftExecutor::AppendEntries(const ballot_t ballot,
                                         shared_ptr<Marshallable> cmd) {
  verify(0);
  return 0;
}

ballot_t RaftExecutor::Decide(ballot_t ballot, CmdData& cmd) {
  verify(0);
  return 0;
}

} // namespace janus
