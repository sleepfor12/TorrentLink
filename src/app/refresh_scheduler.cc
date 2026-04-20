#include "app/refresh_scheduler.h"

#include <QTimer>

namespace pfd::app {

RefreshScheduler::RefreshScheduler(QObject* owner, int intervalMs, RefreshFn refreshFn)
    : refreshFn_(std::move(refreshFn)) {
  timer_ = new QTimer(owner);
  timer_->setSingleShot(true);
  timer_->setInterval(intervalMs);
  QObject::connect(timer_, &QTimer::timeout, owner, [this]() {
    if (!refreshPending_ || !refreshFn_) {
      return;
    }
    refreshPending_ = false;
    refreshFn_();
  });
}

void RefreshScheduler::requestRefresh() {
  refreshPending_ = true;
  if (!timer_->isActive()) {
    timer_->start();
  }
}

void RefreshScheduler::setInterval(int intervalMs) {
  if (timer_ != nullptr) {
    timer_->setInterval(intervalMs);
  }
}

}  // namespace pfd::app
