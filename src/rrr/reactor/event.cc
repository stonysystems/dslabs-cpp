
#include <functional>
#include <thread>
#include "coroutine.h"
#include "event.h"
#include "reactor.h"
#include "epoll_wrapper.h"

namespace rrr {
using std::function;

// void Event::Wait(uint64_t timeout) {
// //  verify(__debug_creator); // if this fails, the event is not created by reactor.

//   verify(Reactor::sp_reactor_th_);
//   verify(Reactor::sp_reactor_th_->thread_id_ == std::this_thread::get_id());
//   if (IsReady()) {
//     status_ = DONE; // does not need to wait.
//     return;
//   } else {
//     verify(status_ == INIT);
//     status_= DEBUG;
//     // the event may be created in a different coroutine.
//     // this value is set when wait is called.
//     // for now only one coroutine can wait on an event.
//     auto sp_coro = Coroutine::CurrentCoroutine();
// //    verify(sp_coro);
// //    verify(_dbg_p_scheduler_ == nullptr);
// //    _dbg_p_scheduler_ = Reactor::GetReactor().get();
//     auto& events = Reactor::GetReactor()->waiting_events_;
//     events.push_back(shared_from_this());
//     wp_coro_ = sp_coro;
//     status_ = WAIT;
//     sp_coro->Yield();
//   }
// }

void Event::Wait(uint64_t timeout) {
//  verify(__debug_creator); // if this fails, the event is not created by reactor.
  auto sp_coro = Coroutine::CurrentCoroutine();
  // Log_info("Event::Wait() called, timeout=%llu, coro_id=%llu, event_status=%d",
          //  timeout, sp_coro ? sp_coro->id : 999999, (int)status_);
  verify(Reactor::sp_reactor_th_);
  verify(Reactor::sp_reactor_th_->thread_id_ == std::this_thread::get_id());
  if (status_ == DONE) {
    // Log_info("Event::Wait() returning early, status is DONE");
    return; // TODO: yidawu add for the second use the event.
  }
  // verify(status_ == INIT);
  if (IsReady()) {
    // Log_info("Event::Wait() event is already ready, marking DONE");
    status_ = DONE; // no need to wait.
    return;
  } else {
//    if (status_ == WAIT) {
//      // this does not look right, fix later
//      Log_fatal("multiple waits on the same event; no support at the moment");
//    }
//    verify(status_ == INIT); // does not support multiple wait so far. maybe we can support it in the future.
//    status_= DEBUG;
    // the event may be created in a different coroutine.
    // this value is set when wait is called.
    // for now only one coroutine can wait on an event.
    verify(sp_coro);
    auto& waiting_events =
          Reactor::GetReactor()->waiting_events_;  // Timeout???
    waiting_events.insert(shared_from_this());
    // Log_info("Event::Wait() added to waiting_events, size now=%zu", waiting_events.size());

    if (timeout > 0) {
      auto now = Time::now(true);
      wakeup_time_ = now + timeout;
      //Log_info("WAITING: %p", shared_from_this());
      // Log_info("Event::Wait() timeout specified, wake up %llu, now %llu", wakeup_time_, now);
      auto& timeout_events = Reactor::GetReactor()->timeout_events_;
      timeout_events.push_back(shared_from_this());
      // Log_info("Event::Wait() added to timeout_events, size now=%zu", timeout_events.size());
    }
    // TODO optimize timeout_events, sort by wakeup time.
//      auto it = timeout_events.end();
//      timeout_events.push_back(shared_from_this());
//      while (it != events.begin()) {
//        it--;
//        auto& it_event = *it;
//        if (it_event->wakeup_time_ < wakeup_time_) {
//          it++; // list insert happens before position.
//          break;
//        }
//      }
//      events.insert(it, shared_from_this());
    wp_coro_ = sp_coro;
    status_ = WAIT;
    // Log_info("Event::Wait() about to call Yield(), coro_id=%llu", sp_coro->id);
    sp_coro->Yield();
    // Log_info("Event::Wait() returned from Yield(), coro_id=%llu", sp_coro->id);
  }
}

bool Event::Test() {
  verify(__debug_creator); // if this fails, the event is not created by reactor.
  if (IsReady()) {
    if (status_ == INIT) {
      // wait has not been called, do nothing until wait happens.
      status_ = DONE;
    } else if (status_ == WAIT) {
      auto sp_coro = wp_coro_.lock();
      verify(sp_coro);
      verify(status_ != DEBUG);
      status_ = READY;
    } else if (status_ == READY) {
      // This could happen for a quorum event.
      Log_debug("event status ready, triggered?");
    } else if (status_ == DONE) {
      // do nothing
    } else if (status_ == TIMEOUT) {
      // Event already marked as timeout by CheckTimeout(), this is OK
      Log_debug("event status already TIMEOUT, this is OK");
    } else {
      Log_fatal("Event::Test() unexpected status: %d", (int)status_);
      verify(0);
    }
    return true;
  }
  return false;
}

Event::Event() {
  auto coro = Coroutine::CurrentCoroutine();
  wp_coro_ = coro;
}

bool IntEvent::TestTrigger() {
  if (status_ > WAIT) {
    Log_debug("Event already triggered!");
    return false;
  }
  if (value_ == target_) {
    if (status_ == INIT) {
      // do nothing until wait happens.
      status_ = DONE;
    } else if (status_ == WAIT) {
      status_ = READY;
    } else {
      verify(0);
    }
    return true;
  }
  return false;
}

int SharedIntEvent::Set(const int& v) {
  auto ret = value_;
  value_ = v;
  for (auto& sp_ev : events_) {
    if (sp_ev->status_ <= Event::WAIT) {
      if (sp_ev->target_ <= v) {
        sp_ev->Set(v);
      }
    }
  }
  return ret;
}

bool SharedIntEvent::WaitUntilGreaterOrEqualThan(int x, int timeout) {
  if (value_ >= x) {
    return false;
  }
  auto sp_ev =  Reactor::CreateSpEvent<IntEvent>();
  sp_ev->value_ = value_;
  sp_ev->target_ = x;
  auto it = events_.insert(events_.end(), sp_ev);
  sp_ev->Wait(timeout);
  // verify(sp_ev->status_ != Event::TIMEOUT);  // why can't it be timeout?
  // remove the event from event vector after it entering a terminate state (READY or TIMEOUT)
  bool if_timeout = (sp_ev->status_ == Event::TIMEOUT);
  events_.erase(it);
  return if_timeout;
}

void SharedIntEvent::Wait(function<bool(int v)> f) {
  if (f(value_)) {
    return;
  }
  auto sp_ev =  Reactor::CreateSpEvent<IntEvent>();
  sp_ev->value_ = value_;
  sp_ev->test_ = f;
  events_.push_back(sp_ev);
  sp_ev->Wait();
}

} // namespace rrr
