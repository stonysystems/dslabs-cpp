#include "frame.h"
#include "commo.h"
#include "scheduler.h"

namespace janus {

REG_FRAME(MODE_NONE_COPILOT, vector<string>({"none_copilot"}), NoneCopilotFrame);

TxLogServer* NoneCopilotFrame::CreateScheduler() {
  auto s = new SchedulerNoneCopilot();
  return s;
}

Communicator* NoneCopilotFrame::CreateCommo(PollMgr* pollmgr) {
  auto comm = new CommunicatorNoneCopilot(pollmgr);
  return comm;
}

}
