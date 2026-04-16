#include "base/throttle.h"

#include <QtCore/QtGlobal>

namespace pfd::base {

Throttler::Throttler(qint64 intervalMs)
    : interval_ms_(intervalMs > 0 ? intervalMs : 1), last_fired_ms_(0), has_fired_(false) {}

bool Throttler::shouldFire(qint64 nowMs) const {
  if (!has_fired_) {
    return true;
  }
  return (nowMs - last_fired_ms_) >= interval_ms_;
}

void Throttler::markFired(qint64 nowMs) {
  last_fired_ms_ = nowMs;
  has_fired_ = true;
}

void Throttler::reset() {
  last_fired_ms_ = 0;
  has_fired_ = false;
}

}  // namespace pfd::base
