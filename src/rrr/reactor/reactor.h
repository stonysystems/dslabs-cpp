#pragma once
#include <algorithm>
#include <list>
#include <memory>
#include <set>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <rusty/rusty.hpp>
#include <rusty/thread.hpp>
#include <rusty/arc.hpp>
#include <rusty/mutex.hpp>
#include "base/misc.hpp"
#include "event.h"
#include "quorum_event.h"
#include "coroutine.h"
#include "epoll_wrapper.h"

// External safety annotations for system functions used in this module
// @external: {
//   pthread_setname_np: [unsafe, (pthread_t, const char*) -> int]
// }

// External safety annotations for STL operations
// @external: {
//   operator!=: [safe, (auto, auto) -> bool]
//   operator==: [safe, (auto, auto) -> bool]
//   std::*::find: [safe, (auto) -> auto]
//   std::*::end: [safe, () -> auto]
// }

namespace rrr {

using std::make_unique;
using std::make_shared;

class Coroutine;
// TODO for now we depend on the rpc services, fix in the future.
// @safe - Thread-safe reactor with thread-local storage
class Reactor {
 public:
  // @safe - Returns thread-local reactor instance
  static std::shared_ptr<Reactor> GetReactor();
  static std::shared_ptr<Reactor> GetDiskReactor();
  static thread_local std::shared_ptr<Reactor> sp_reactor_th_;
  static thread_local std::shared_ptr<Reactor> sp_disk_reactor_th_;
  static thread_local std::shared_ptr<Coroutine> sp_running_coro_th_;
  int server_id_;

  struct thread_params{
		std::string ip_;
	};
  /*struct eventComp{
    bool operator()(const std::shared_ptr<Event>& lhs, const std::shared_ptf<Event>& rhs) const{
      return lhs->timeout
    }
  }*/

  /**
   * A reactor needs to keep reference to all coroutines created,
   * in case it is freed by the caller after a yield.
   */
  std::list<std::shared_ptr<Event>> all_events_{};
  std::set<std::shared_ptr<Event>> waiting_events_{};
  std::vector<std::shared_ptr<Event>> ready_events_{};
  std::list<std::shared_ptr<Event>> timeout_events_{};
  std::vector<std::shared_ptr<Event>> disk_events_{};
  std::list<std::shared_ptr<Event>> ready_disk_events_{};
  std::vector<std::shared_ptr<Event>> network_events_{};
  std::list<std::shared_ptr<Event>> ready_network_events_{};
  std::set<std::shared_ptr<Coroutine>> coros_{};
  std::vector<std::shared_ptr<Coroutine>> available_coros_{};
  std::unordered_map<uint64_t, std::function<void(Event&)>> processors_{};
  std::unordered_map<std::string, FILE*> opened_files_{};
	static thread_local std::unordered_map<std::string, std::vector<std::shared_ptr<rrr::Pollable>>> clients_;
	static thread_local std::unordered_set<std::string> dangling_ips_;
  bool looping_{false};
	bool slow_{false};
	long disk_times[50];
	int disk_count{0};
	int disk_index{0};
	int slow_count{0};
	int trying_count{0};
  std::thread::id thread_id_{};
  int64_t n_created_coroutines_{0};
  int64_t n_busy_coroutines_{0};
  int64_t n_active_coroutines_{0};
  int64_t n_active_coroutines_2_{0};
  int64_t n_idle_coroutines_{0};
  static SpinLock disk_job_;
	static SpinLock trying_job_;
#ifdef REUSE_CORO
#define REUSING_CORO (true)
#else
#define REUSING_CORO (false)
#endif

  // @safe - Checks and processes timeout events
  void CheckTimeout(std::vector<std::shared_ptr<Event>>&);
  /**
   * @param ev. is usually allocated on coroutine stack. memory managed by user.
   */
  // @safe - Creates and runs a new coroutine
  // HYBRID: Use move_only_function but preserve debug parameters
  std::shared_ptr<Coroutine> CreateRunCoroutine(std::move_only_function<void()> func, const char *file="", int64_t line=0);
  // @safe - Main event loop
  void Loop(bool infinite = false, bool check_timeout = false);
	void DiskLoop();
	void NetworkLoop();
  // @safe - Continues execution of a paused coroutine
  void ContinueCoro(std::shared_ptr<Coroutine> sp_coro);
  void Recycle(std::shared_ptr<Coroutine>& sp_coro);
  void DisplayWaitingEv();

  ~Reactor() {
//    verify(0);
  }
  friend Event;

  
  // @safe - Creates shared_ptr event with perfect forwarding
  template <typename Ev, typename... Args>
  static shared_ptr<Ev> CreateSpEvent(Args&&... args) {
    auto sp_ev = make_shared<Ev>(args...);
    sp_ev->__debug_creator = 1;
    // TODO push them into a wait queue when they actually wait.
    //events.push_back(sp_ev);
    //Log_info("ADDING %s %d", typeid(sp_ev).name(), events.size());
    return sp_ev;
  }

  // @safe - Creates event and returns reference
  template <typename Ev, typename... Args>
  static Ev& CreateEvent(Args&&... args) {
    auto sp_ev = CreateSpEvent<Ev>(args...);
    return *sp_ev;
  }
};

// @safe - Thread-safe polling thread with automatic memory management
// HYBRID: Keep mako-dev's PollThreadWorker architecture
class PollThreadWorker {
    // Friend Arc to allow make_in_place access to private constructor
    friend class rusty::Arc<PollThreadWorker>;

private:
    // All members are mutable to allow const methods with interior mutability
    mutable Epoll poll_{};

    // Wrap non-movable SpinLocks in unique_ptr to make class movable
    mutable std::unique_ptr<SpinLock> l_;
    // Authoritative storage: fd -> shared_ptr<Pollable>
    mutable std::unordered_map<int, std::shared_ptr<Pollable>> fd_to_pollable_;
    mutable std::unordered_map<int, int> mode_; // fd->mode

    mutable std::set<std::shared_ptr<Job>> set_sp_jobs_;

    mutable std::unordered_set<int> pending_remove_;  // Store fds to remove
    mutable std::unique_ptr<SpinLock> pending_remove_l_;
    mutable std::unique_ptr<SpinLock> lock_job_;

    mutable rusty::Option<rusty::thread::JoinHandle<void>> join_handle_;
    mutable std::unique_ptr<std::atomic<bool>> stop_flag_;  // Wrap atomic to make movable

    // Private constructor - use create() factory
    PollThreadWorker();

    // @unsafe - Triggers ready jobs in coroutines
    // SAFETY: Uses spinlock for thread safety
    void TriggerJob() const;

public:
    ~PollThreadWorker();

    // Factory method returns Arc<PollThreadWorker>
    static rusty::Arc<PollThreadWorker> create();

    // Member function for thread - not static!
    void poll_loop() const;

    // Explicit shutdown (replaces RAII)
    void shutdown() const;

    PollThreadWorker(const PollThreadWorker&) = delete;
    PollThreadWorker& operator=(const PollThreadWorker&) = delete;

    // Explicit move constructors
    PollThreadWorker(PollThreadWorker&& other) noexcept
        : poll_(std::move(other.poll_)),
          l_(std::move(other.l_)),
          fd_to_pollable_(std::move(other.fd_to_pollable_)),
          mode_(std::move(other.mode_)),
          set_sp_jobs_(std::move(other.set_sp_jobs_)),
          pending_remove_(std::move(other.pending_remove_)),
          pending_remove_l_(std::move(other.pending_remove_l_)),
          lock_job_(std::move(other.lock_job_)),
          join_handle_(std::move(other.join_handle_)),
          stop_flag_(std::move(other.stop_flag_)) {}

    PollThreadWorker& operator=(PollThreadWorker&& other) noexcept {
        if (this != &other) {
            poll_ = std::move(other.poll_);
            l_ = std::move(other.l_);
            fd_to_pollable_ = std::move(other.fd_to_pollable_);
            mode_ = std::move(other.mode_);
            set_sp_jobs_ = std::move(other.set_sp_jobs_);
            pending_remove_ = std::move(other.pending_remove_);
            pending_remove_l_ = std::move(other.pending_remove_l_);
            lock_job_ = std::move(other.lock_job_);
            join_handle_ = std::move(other.join_handle_);
            stop_flag_ = std::move(other.stop_flag_);
        }
        return *this;
    }

    // @safe - Thread-safe addition of pollable object
    void add(std::shared_ptr<Pollable> poll) const;
    // @safe - Thread-safe removal of pollable object
    void remove(Pollable& poll) const;
    // @safe - Thread-safe mode update
    void update_mode(Pollable& poll, int new_mode) const;

    // Frequent Job
    // @safe - Thread-safe job management
    void add(std::shared_ptr<Job> sp_job) const;
    void remove(std::shared_ptr<Job> sp_job) const;

    // For testing: get number of epoll Remove() calls
    int get_remove_count() const { return poll_.remove_count_.load(); }

    // Stub for test compatibility - simulates network slow down (not implemented)
    void slow(uint32_t usec) const { /* Not implemented in mako-dev */ }
};

} // namespace rrr

// Trait specializations for PollThreadWorker
// PollThreadWorker is Send + Sync because:
// - All methods are const with interior mutability via internal SpinLocks
// - All members are mutable
// - Designed for thread-safe concurrent access
namespace rusty {
template<>
struct is_send<rrr::PollThreadWorker> : std::true_type {};

template<>
struct is_sync<rrr::PollThreadWorker> : std::true_type {};
} // namespace rusty
