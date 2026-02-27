#pragma once


#define USE_BOOST_COROUTINE2

#ifdef USE_BOOST_COROUTINE2
#define BOOST_COROUTINE_NO_DEPRECATION_WARNING 1
#define BOOST_COROUTINES_NO_DEPRECATION_WARNING 1
#include <boost/coroutine2/all.hpp>
#endif

#ifdef USE_BOOST_COROUTINE1
#include <boost/coroutine/symmetric_coroutine.hpp>
#endif

#include <boost/optional.hpp>
#include <vector>

//#include <experimental/coroutine>

namespace rrr {

#ifdef USE_BOOST_COROUTINE2
typedef boost::coroutines2::coroutine<void>::pull_type boost_coro_task_t;
typedef boost::coroutines2::coroutine<void>::push_type boost_coro_yield_t;
typedef boost::coroutines2::coroutine<void()> coro_t;
#endif

#ifdef USE_BOOST_COROUTINE1
    typedef boost::coroutines::symmetric_coroutine<void>::call_type boost_coro_task_t;
    typedef boost::coroutines::symmetric_coroutine<void>::yield_type boost_coro_yield_t;
    typedef boost::coroutines::symmetric_coroutine<void()> coro_t;
#endif

class Reactor;
class Event;
class Coroutine {
 public:
  static std::shared_ptr<Coroutine> CurrentCoroutine();
  // the argument cannot be a reference because it could be declared on stack.
  // HYBRID: Using std::move_only_function to support move-only callables (e.g., lambdas capturing rusty::Box)
  //         BUT preserving lab-solution's debug parameters (file, line)
  static std::shared_ptr<Coroutine> CreateRun(std::move_only_function<void()> func, const char *file="", int64_t line=0);
  static void Sleep(uint64_t microseconds);
  static uint64_t global_id;
  uint64_t dep_id_;
  bool need_finalize_;
  uint64_t id;

  enum Status {INIT=0, STARTED, PAUSED, RESUMED, FINISHED, FINALIZING, RECYCLED};

  Status status_ = INIT; //
  bool needs_finalize_ = false;
  std::move_only_function<void()> func_{};

  std::shared_ptr<boost_coro_task_t> up_boost_coro_task_{nullptr};
  boost::optional<boost_coro_yield_t&> boost_coro_yield_{};

  Coroutine() = delete;
  Coroutine(std::move_only_function<void()> func);
  ~Coroutine();
  void BoostRunWrapper(boost_coro_yield_t& yield);
  void Run();
  void Yield();
  void Continue();
  bool Finished();
	void DoFinalize();
};

} // namespace rrr
