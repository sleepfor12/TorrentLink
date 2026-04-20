#ifndef PFD_APP_REFRESH_SCHEDULER_H
#define PFD_APP_REFRESH_SCHEDULER_H

#include <functional>

class QTimer;
class QObject;

namespace pfd::app {

class RefreshScheduler {
public:
  using RefreshFn = std::function<void()>;

  RefreshScheduler(QObject* owner, int intervalMs, RefreshFn refreshFn);

  void requestRefresh();
  void setInterval(int intervalMs);

private:
  QTimer* timer_{nullptr};
  bool refreshPending_{false};
  RefreshFn refreshFn_;
};

}  // namespace pfd::app

#endif  // PFD_APP_REFRESH_SCHEDULER_H
