
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "../base/all.hpp"
#include "reactor.h"
#include "coroutine.h"
#include "event.h"
#include "epoll_wrapper.h"

namespace rrr {

thread_local std::shared_ptr<Reactor> Reactor::sp_reactor_th_{};
thread_local std::shared_ptr<Coroutine> Reactor::sp_running_coro_th_{};

// @safe - Returns current thread-local coroutine
std::shared_ptr<Coroutine> Coroutine::CurrentCoroutine() {
  // TODO re-enable this verify
//  verify(sp_running_coro_th_);
  return Reactor::sp_running_coro_th_;
}

// @unsafe - Creates and runs a new coroutine with function wrapping
// SAFETY: Reactor manages coroutine lifecycle properly
// HYBRID: Use move_only_function but preserve debug parameters
std::shared_ptr<Coroutine>
Coroutine::CreateRun(std::move_only_function<void()> func, const char* file, int64_t line) {
  auto& reactor = *Reactor::GetReactor();
  auto coro = reactor.CreateRunCoroutine(std::move(func), file, line);
  // some events might be triggered in the last coroutine.
  return coro;
}

// @unsafe - Returns thread-local reactor instance, creates if needed
// SAFETY: Thread-local storage ensures thread safety
std::shared_ptr<Reactor>
Reactor::GetReactor() {
  if (!sp_reactor_th_) {
    Log_debug("create a coroutine scheduler");
    sp_reactor_th_ = std::make_shared<Reactor>();
    sp_reactor_th_->thread_id_ = std::this_thread::get_id();
  }
  return sp_reactor_th_;
}

/**
 * @param func
 * @return
 */
// @unsafe - Creates and runs coroutine with complex state management
// SAFETY: Proper lifecycle management with shared_ptr
// HYBRID: Use move_only_function but preserve debug parameters and logging
std::shared_ptr<Coroutine>
Reactor::CreateRunCoroutine(std::move_only_function<void()> func, const char *file, int64_t line) {
  std::shared_ptr<Coroutine> sp_coro;
  const bool reusing = REUSING_CORO && !available_coros_.empty();
  if (reusing) {
    n_idle_coroutines_--;
    sp_coro = available_coros_.back();
    sp_coro->id = Coroutine::global_id++;
    available_coros_.pop_back();
    sp_coro->func_ = std::move(func);
  } else {
    sp_coro = std::make_shared<Coroutine>(std::move(func));
    n_created_coroutines_++;
    if (n_created_coroutines_ % 1024 == 0) {
      Log_debug("created %ld, busy %ld, idle %ld coroutines on server %d, "
               "recent %s:%lld",
               (long)n_created_coroutines_,
               (long)n_busy_coroutines_,
               (long)n_idle_coroutines_,
               server_id_,
               file, line);
    }
  }
  
  // Save old coroutine context
  auto sp_old_coro = sp_running_coro_th_;
  sp_running_coro_th_ = sp_coro;
  
  verify(sp_coro);
  auto pair = coros_.insert(sp_coro);
  verify(pair.second);
  verify(coros_.size() > 0);
  
  sp_coro->Run();
  if (sp_coro->Finished()) {
    coros_.erase(sp_coro);
  }
  
  Loop();
  
  // yielded or finished, reset to old coro.
  sp_running_coro_th_ = sp_old_coro;
  return sp_coro;
}

// @safe - Checks timeout events and moves ready ones to ready list
void Reactor::CheckTimeout(std::vector<std::shared_ptr<Event>>& ready_events ) {
  auto time_now = Time::now(true);
  for (auto it = timeout_events_.begin(); it != timeout_events_.end();) {
    Event& event = **it;
    auto status = event.status_;
    switch (status) {
      case Event::INIT:
        verify(0);
      case Event::WAIT: {
        const auto &wakeup_time = event.wakeup_time_;
        verify(wakeup_time > 0);
        if (time_now > wakeup_time) {
          if (event.IsReady()) {
            // This is because our event mechanism is not perfect, some events
            // don't get triggered with arbitrary condition change.
            event.status_ = Event::READY;
          } else {
            event.status_ = Event::TIMEOUT;
          }
          ready_events.push_back(*it);
          it = timeout_events_.erase(it);
        } else {
          it++;
        }
        break;
      }
      case Event::READY:
      case Event::DONE:
        it = timeout_events_.erase(it);
        break;
      default:
        verify(0);
    }
  }

}

//  be careful this could be called from different coroutines.
// @unsafe - Main event loop with complex event processing
// SAFETY: Thread-safe via thread_id verification
void Reactor::Loop(bool infinite, bool check_timeout) {
  verify(std::this_thread::get_id() == thread_id_);
  looping_ = infinite;
  do {
    // Keep processing events until no new ready events are found
    // This fixes the event chain propagation issue
    bool found_ready_events = true;
    while (found_ready_events) {
      found_ready_events = false;
      std::vector<shared_ptr<Event>> ready_events;
      
      // Check waiting events
      auto& events = waiting_events_;
      for (auto it = events.begin(); it != events.end();) {
        Event& event = **it;
        event.Test();
        if (event.status_ == Event::READY) {
          ready_events.push_back(std::move(*it));
          it = events.erase(it);
          found_ready_events = true;
        } else if (event.status_ == Event::DONE) {
          it = events.erase(it);
        } else {
          it ++;
        }
      }
      
      CheckTimeout(ready_events);
      
      // Process ready events
      for (auto& up_ev: ready_events) {
        auto& event = *up_ev;
        auto sp_coro = event.wp_coro_.lock();
        verify(sp_coro);
        verify(coros_.find(sp_coro) != coros_.end());
        if (event.status_ == Event::READY) {
          event.status_ = Event::DONE;
        } else {
          verify(event.status_ == Event::TIMEOUT);
        }
        ContinueCoro(sp_coro);
      }
      
      // If we're not in infinite mode and found no events, stop inner loop
      if (!infinite && !found_ready_events) {
        break;
      }
    }
  } while (looping_);
}

// @unsafe - Continues execution of paused coroutine
// SAFETY: Manages coroutine state transitions properly
void Reactor::ContinueCoro(std::shared_ptr<Coroutine> sp_coro) {
//  verify(!sp_running_coro_th_); // disallow nested coros
  auto sp_old_coro = sp_running_coro_th_;
  sp_running_coro_th_ = sp_coro;
  verify(!sp_running_coro_th_->Finished());
  if (sp_coro->status_ == Coroutine::INIT) {
    sp_coro->Run();
  } else {
    // PAUSED or RECYCLED
    sp_running_coro_th_->Continue();
  }
  if (sp_running_coro_th_->Finished()) {
    if (REUSING_CORO) {
      sp_coro->status_ = Coroutine::RECYCLED;
      available_coros_.push_back(sp_running_coro_th_);
    }
    coros_.erase(sp_running_coro_th_);
  }
  sp_running_coro_th_ = sp_old_coro;
}

// TODO PollThreadWorker -> Reactor

// Private constructor - doesn't start thread
PollThreadWorker::PollThreadWorker()
    : l_(std::make_unique<SpinLock>()),
      pending_remove_l_(std::make_unique<SpinLock>()),
      lock_job_(std::make_unique<SpinLock>()),
      stop_flag_(std::make_unique<std::atomic<bool>>(false)) {
  // Don't start thread here - factory will do it
}

// Factory method creates Arc<PollThreadWorker> and starts thread
rusty::Arc<PollThreadWorker> PollThreadWorker::create() {
  // Create Arc directly (methods are const, so no need for Mutex)
  auto arc = rusty::Arc<PollThreadWorker>::new_(PollThreadWorker());

  // Clone Arc for thread
  auto thread_arc = arc.clone();

  // Spawn thread with explicit parameter passing (enforces Send trait checking)
  // This properly validates that Arc<PollThreadWorker> is Send
  auto handle = rusty::thread::spawn(
    [](rusty::Arc<PollThreadWorker> arc) {
      arc->poll_loop();
    },
    thread_arc
  );

  // Store handle (using const method)
  arc->join_handle_ = rusty::Some(std::move(handle));

  return arc;
}

// Explicit shutdown method
void PollThreadWorker::shutdown() const {
  // Remove pollables before stopping
  for (auto& pair : fd_to_pollable_) {
    this->remove(*pair.second);
  }

  // Signal thread to stop
  stop_flag_->store(true);

  // Join thread
  if (join_handle_.is_some()) {
    join_handle_.take().unwrap().join();
  }
}

// Destructor just warns if not shut down
PollThreadWorker::~PollThreadWorker() {
  // Check if stop_flag_ is not null (it may be null if object was moved)
  if (stop_flag_ && !stop_flag_->load()) {
    Log_error("PollThreadWorker destroyed without shutdown() - thread may leak!");
  }
}

// @unsafe - Triggers ready jobs in coroutines
// SAFETY: Uses spinlock for thread safety
// HYBRID: Keep debug parameters for coroutine creation
void PollThreadWorker::TriggerJob() const {
  lock_job_->lock();
  auto jobs_exec = set_sp_jobs_;
  set_sp_jobs_.clear();
  lock_job_->unlock();
  auto it = jobs_exec.begin();
  while (it != jobs_exec.end()) {
    auto sp_job = *it;
    if (sp_job->Ready()) {
      Coroutine::CreateRun([sp_job]() {sp_job->Work();}, __FILE__, __LINE__);
      it = jobs_exec.erase(it);
    }
    else {
      it++;
    }
  }
}

// @unsafe - Main polling loop with complex synchronization
// SAFETY: Uses spinlocks and proper synchronization primitives
void PollThreadWorker::poll_loop() const {
  while (!stop_flag_->load()) {
    TriggerJob();
    // Wait() now directly casts userdata to Pollable* and calls handlers
    // Safe because deferred removal guarantees object stays in fd_to_pollable_ map
    poll_.Wait();
    TriggerJob();

    // Process deferred removals AFTER all events handled
    pending_remove_l_->lock();
    std::unordered_set<int> remove_fds = std::move(pending_remove_);
    pending_remove_.clear();
    pending_remove_l_->unlock();

    for (int fd : remove_fds) {
      l_->lock();

      auto it = fd_to_pollable_.find(fd);
      if (it == fd_to_pollable_.end()) {
        l_->unlock();
        continue;
      }

      auto sp_poll = it->second;

      // Check if fd was NOT reused (still in mode_ map)
      if (mode_.find(fd) != mode_.end()) {
        poll_.Remove(sp_poll);
      }

      // Remove from map - object may be destroyed here
      fd_to_pollable_.erase(it);
      mode_.erase(fd);

      l_->unlock();
    }
    TriggerJob();
    Reactor::GetReactor()->Loop();
    verify(Reactor::GetReactor()->ready_events_.empty());
  }

  // Process any final pending removals after stop_flag_ is set
  // This ensures destructor cleanup is processed even if the thread
  // exits the loop before processing the last batch
  pending_remove_l_->lock();
  std::unordered_set<int> remove_fds = std::move(pending_remove_);
  pending_remove_.clear();
  pending_remove_l_->unlock();

  for (int fd : remove_fds) {
    l_->lock();

    auto it = fd_to_pollable_.find(fd);
    if (it == fd_to_pollable_.end()) {
      l_->unlock();
      continue;
    }

    auto sp_poll = it->second;

    // Check if fd was NOT reused (still in mode_ map)
    if (mode_.find(fd) != mode_.end()) {
      poll_.Remove(sp_poll);
    }

    // Remove from map - object may be destroyed here
    fd_to_pollable_.erase(it);
    mode_.erase(fd);

    l_->unlock();
  }
}

// @safe - Thread-safe job addition with spinlock
void PollThreadWorker::add(std::shared_ptr<Job> sp_job) const {
  lock_job_->lock();
  set_sp_jobs_.insert(sp_job);
  lock_job_->unlock();
}

// @safe - Thread-safe job removal with spinlock
void PollThreadWorker::remove(std::shared_ptr<Job> sp_job) const {
  lock_job_->lock();
  set_sp_jobs_.erase(sp_job);
  lock_job_->unlock();
}

// @safe - Adds pollable with shared_ptr ownership
// SAFETY: Stores shared_ptr in map, passes raw pointer to epoll
void PollThreadWorker::add(std::shared_ptr<Pollable> sp_poll) const {
  int fd = sp_poll->fd();
  int poll_mode = sp_poll->poll_mode();

  l_->lock();

  // Check if already exists
  if (fd_to_pollable_.find(fd) != fd_to_pollable_.end()) {
    l_->unlock();
    return;
  }

  // Store in map
  fd_to_pollable_[fd] = sp_poll;
  mode_[fd] = poll_mode;

  // userdata = raw Pollable* for lookup
  void* userdata = sp_poll.get();

  poll_.Add(sp_poll, userdata);

  l_->unlock();
}

// @unsafe - Removes pollable with deferred cleanup
// SAFETY: Deferred removal ensures safe cleanup
void PollThreadWorker::remove(Pollable& poll) const {
  int fd = poll.fd();

  l_->lock();
  if (fd_to_pollable_.find(fd) == fd_to_pollable_.end()) {
    l_->unlock();
    return;  // Not found
  }
  l_->unlock();

  // Add to pending_remove (actual removal happens after epoll_wait)
  pending_remove_l_->lock();
  pending_remove_.insert(fd);
  pending_remove_l_->unlock();
}

// @unsafe - Updates poll mode
// SAFETY: Protected by spinlock, validates poll existence
void PollThreadWorker::update_mode(Pollable& poll, int new_mode) const {
  int fd = poll.fd();
  l_->lock();

  // Verify the pollable is registered
  if (fd_to_pollable_.find(fd) == fd_to_pollable_.end()) {
    l_->unlock();
    return;
  }

  auto mode_it = mode_.find(fd);
  verify(mode_it != mode_.end());
  int old_mode = mode_it->second;
  mode_it->second = new_mode;

  if (new_mode != old_mode) {
    void* userdata = &poll;  // Use address of reference
    poll_.Update(poll, userdata, new_mode, old_mode);
  }

  l_->unlock();
}

} // namespace rrr
