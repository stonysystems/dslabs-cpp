
#include "../__dep__.h"
#include "../constants.h"
#include "coordinator.h"
#include "commo.h"

#include "server.h"

namespace janus {

CoordinatorRaft::CoordinatorRaft(uint32_t coo_id,
                                             int32_t benchmark,
                                             ClientControlServiceImpl* ccsi,
                                             uint32_t thread_id)
    : Coordinator(coo_id, benchmark, ccsi, thread_id) {
}

bool CoordinatorRaft::IsLeader() {
   return this->svr_->IsLeader() ;
}

bool CoordinatorRaft::IsFPGALeader() {
   return this->svr_->IsFPGALeader() ;
}

void CoordinatorRaft::Submit(shared_ptr<Marshallable>& cmd,
                                   const function<void()>& func,
                                   const function<void()>& exe_callback) {
  verify(0);
  if (!IsLeader()) {
    verify(0);
    return ;
  }
  
	std::lock_guard<std::recursive_mutex> lock(mtx_);
  verify(!in_submission_);
  verify(cmd_ == nullptr);
//  verify(cmd.self_cmd_ != nullptr);
  in_submission_ = true;
  cmd_ = cmd;
  verify(cmd_->kind_ != MarshallDeputy::UNKNOWN);
  commit_callback_ = func;
  GotoNextPhase();
}

void CoordinatorRaft::AppendEntries() {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    verify(!in_append_entries);
    // verify(this->svr_->IsLeader()); TODO del it yidawu
    in_append_entries = true;
    uint64_t index, term;

    bool ok = this->svr_->Start(cmd_, &index, &term); //, slot_id_, curr_ballot_);
    verify(ok);

    // while (this->sch_->commitIndex < index) {
    //   Reactor::CreateSpEvent<TimeoutEvent>(1000)->Wait();
    //   verify(this->sch_->currentTerm == term);
    // }
    committed_ = true;
}

void CoordinatorRaft::Commit() {
  verify(0);
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  commit_callback_();
  verify(phase_ == Phase::COMMIT);
  GotoNextPhase();
}

void CoordinatorRaft::LeaderLearn() {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    commit_callback_();
    verify(phase_ == Phase::COMMIT);
    GotoNextPhase();
}

void CoordinatorRaft::GotoNextPhase() {
  int n_phase = 4;
  int current_phase = phase_ % n_phase;
  phase_++;
  switch (current_phase) {
    case Phase::INIT_END:
      if (IsLeader()) {
        phase_++; // skip prepare phase for "leader"
        verify(phase_ % n_phase == Phase::ACCEPT);
        AppendEntries();
        phase_++;
        verify(phase_ % n_phase == Phase::COMMIT);
      } else {
        // TODO
        verify(0);
        // Forward(cmd_,commit_callback_) ;
        phase_ = Phase::COMMIT;
      }
    case Phase::ACCEPT:
      verify(phase_ % n_phase == Phase::COMMIT);
      if (committed_) {
        LeaderLearn();
      } else {
        // verify(0);
        // Forward(cmd_,commit_callback_) ;
        phase_ = Phase::COMMIT;
      }
      break;
    case Phase::PREPARE:
      verify(phase_ % n_phase == Phase::ACCEPT);
      AppendEntries();
      break;
    case Phase::COMMIT:
      // do nothing.
      break;
    default:
      verify(0);
  }
}

} // namespace janus
