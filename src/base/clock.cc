#include "base/clock.h"

#include "base/time_stamps.h"

namespace pfd::base {

qint64 Clock::nowMs() const {
  if (provider_) {
    return provider_();
  }
  return currentMs();
}

void Clock::setNowMsProvider(NowMsFn fn) {
  provider_ = std::move(fn);
}

void Clock::reset() {
  provider_ = nullptr;
}

}  // namespace pfd::base
