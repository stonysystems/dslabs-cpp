/**
 * @file fiber.h
 * @brief Fiber naming compatibility layer.
 *
 * This project still uses the legacy Coroutine implementation internally,
 * but we expose a Fiber-facing API to align terminology with mako-dev.
 */

#pragma once

#include "coroutine.h"
#include "../base/all.hpp"

namespace rrr {

// Naming alias: Fiber is the preferred public term.
using Fiber = Coroutine;

namespace this_fiber {

inline uint64_t get_id() noexcept {
  auto fiber = Fiber::CurrentCoroutine();
  return fiber ? fiber->id : 0;
}

inline std::shared_ptr<Fiber> current() noexcept {
  return Fiber::CurrentCoroutine();
}

inline bool in_fiber_context() noexcept {
  return static_cast<bool>(Fiber::CurrentCoroutine());
}

inline void yield() noexcept {
  auto fiber = Fiber::CurrentCoroutine();
  if (fiber) {
    fiber->Yield();
  }
}

inline void sleep_us(uint64_t microseconds) {
  Fiber::Sleep(microseconds);
}

inline void sleep_ms(uint64_t milliseconds) {
  Fiber::Sleep(milliseconds * 1000);
}

inline void sleep_s(uint64_t seconds) {
  Fiber::Sleep(seconds * Time::RRR_USEC_PER_SEC);
}

inline void sleep_until_us(uint64_t abs_time_us) {
  uint64_t now = Time::now(true);
  if (abs_time_us > now) {
    Fiber::Sleep(abs_time_us - now);
  }
}

}  // namespace this_fiber

}  // namespace rrr
