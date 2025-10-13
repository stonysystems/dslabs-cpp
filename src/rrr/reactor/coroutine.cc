#include <utility>

#include <functional>
#include <iostream>
#include <boost/coroutine2/protected_fixedsize_stack.hpp>
#include "../base/all.hpp"
#include "coroutine.h"
#include "reactor.h"

// #define USE_PROTECTED_STACK

namespace rrr {
uint64_t Coroutine::global_id = 0;

// HYBRID: Use move_only_function but preserve global_id tracking
Coroutine::Coroutine(std::move_only_function<void()> func) : func_(std::move(func)), status_(INIT), id(Coroutine::global_id++) {
}

Coroutine::~Coroutine() {
  // Don't verify - coroutines in thread-local storage cleanup
  // can be in various intermediate states
  if (up_boost_coro_task_) {
    up_boost_coro_task_.reset();
  }
//  verify(0);
}

void Coroutine::BoostRunWrapper(boost_coro_yield_t& yield) {
  boost_coro_yield_ = yield;
  verify(func_);
  auto reactor = Reactor::GetReactor();
//  reactor->coros_;
  while (true) {
    auto sz = reactor->coros_.size();
    verify(sz > 0);
    verify(func_);
    func_();
//    func_ = nullptr; // Can be swapped out here?
		status_ = FINISHED;
		if (needs_finalize_) {
			Log_info("Warning: We did not deal with backlog issues");
			needs_finalize_ = false;
		}
    Reactor::GetReactor()->n_active_coroutines_--;
    yield();
  }
}

void Coroutine::Run() {
  verify(!up_boost_coro_task_);
  verify(status_ == INIT);
  status_ = STARTED;
  auto reactor = Reactor::GetReactor();
//  reactor->coros_;
  auto sz = reactor->coros_.size();
  verify(sz > 0);
//  up_boost_coro_task_ = make_shared<boost_coro_task_t>(

  const auto x = new boost_coro_task_t(
#ifdef USE_PROTECTED_STACK
      boost::coroutines2::protected_fixedsize_stack(boost::context::stack_traits::default_size() * 2),
      // ATTENTION: STACK MEMORY IS PRECIOUS, AVOID EXCESSIVE RECURSION CALL
#else
      boost::coroutines2::default_stack(boost::context::stack_traits::default_size() * 2),
#endif
      std::bind(&Coroutine::BoostRunWrapper, this, std::placeholders::_1)
//    [this] (boost_coro_yield_t& yield) {
//      this->BoostRunWrapper(yield);
//    }
      );
  verify(up_boost_coro_task_ == nullptr);
  up_boost_coro_task_.reset(x);
#ifdef USE_BOOST_COROUTINE1
  (*up_boost_coro_task_)();
#endif
}

void Coroutine::Yield() {
  // Log_info("Coroutine::Yield() called, coro_id=%llu, status=%d", id, (int)status_);
  verify(boost_coro_yield_);
  verify(status_ == STARTED || status_ == RESUMED || status_ == FINALIZING);
  status_ = PAUSED;
  Reactor::GetReactor()->n_active_coroutines_--;
  // Log_info("Coroutine::Yield() about to call boost_coro_yield_, coro_id=%llu", id);
  boost_coro_yield_.value()();
  // Log_info("Coroutine::Yield() returned from boost_coro_yield_, coro_id=%llu", id);
}

void Coroutine::Continue() {
  // Log_info("Coroutine::Continue() called, coro_id=%llu, status=%d", id, (int)status_);
  verify(status_ == PAUSED || status_ == RECYCLED);
  verify(up_boost_coro_task_);
  status_ = RESUMED;
  auto& r = *up_boost_coro_task_;
  verify(r);
  // Log_info("Coroutine::Continue() about to resume boost coroutine, coro_id=%llu", id);
  r();
  // Log_info("Coroutine::Continue() returned from boost coroutine, coro_id=%llu", id);
  // some events might have been triggered from last coroutine,
  // but you have to manually call the scheduler to loop.
}

bool Coroutine::Finished() {
  return status_ == FINISHED;
}

void Coroutine::Sleep(uint64_t microseconds) {
  auto x = Reactor::CreateSpEvent<TimeoutEvent>(microseconds);
  x->Wait();
}

} // namespace rrr
