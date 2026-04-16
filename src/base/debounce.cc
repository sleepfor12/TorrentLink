#include "base/debounce.h"

#include <QtCore/QTimer>

namespace pfd::base {

Debouncer::Debouncer(QObject* parent, qint64 delayMs)
    : QObject(parent), delay_ms_(delayMs > 0 ? delayMs : 1), timer_(this) {
  timer_.setSingleShot(true);
  connect(&timer_, &QTimer::timeout, this, &Debouncer::onTimeout);
}

void Debouncer::schedule(std::function<void()> fn) {
  pending_fn_ = std::move(fn);
  timer_.stop();
  timer_.setInterval(static_cast<int>(delay_ms_));
  timer_.start();
}

void Debouncer::cancel() {
  timer_.stop();
  pending_fn_ = nullptr;
}

void Debouncer::setDelay(qint64 delayMs) {
  delay_ms_ = delayMs > 0 ? delayMs : 1;
}

void Debouncer::onTimeout() {
  if (pending_fn_) {
    auto fn = std::move(pending_fn_);
    pending_fn_ = nullptr;
    fn();
  }
}

}  // namespace pfd::base
