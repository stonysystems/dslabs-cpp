
#include "../copilot/frame.h"

namespace janus {
class NoneCopilotFrame : public CopilotFrame {
  using CopilotFrame::CopilotFrame;
  TxLogServer *CreateScheduler() override;
  Communicator* CreateCommo(PollMgr* pollmgr) override;
};

}
