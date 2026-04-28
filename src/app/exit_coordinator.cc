#include "app/exit_coordinator.h"

#include <QtWidgets/QApplication>

namespace pfd::app {

void ExitCoordinator::beginShutdown(std::atomic<bool>& shuttingDown) const {
  shuttingDown.store(true);
}

void ExitCoordinator::wireAboutToQuit(QApplication* app, QObject* context,
                                      const std::function<void()>& fastPersistFn) const {
  QObject::connect(app, &QApplication::aboutToQuit, context, [fastPersistFn]() {
    if (fastPersistFn) {
      fastPersistFn();
    }
  });
}

void ExitCoordinator::waitFutureAtMost(std::future<void>& fut,
                                       std::chrono::milliseconds timeout) const {
  if (!fut.valid()) {
    return;
  }
  if (fut.wait_for(timeout) == std::future_status::ready) {
    fut.wait();
  }
}

void ExitCoordinator::waitBackgroundTasks(std::vector<std::future<void>>& tasks,
                                          std::future<void>& statusTask,
                                          std::chrono::milliseconds timeout) const {
  for (auto& f : tasks) {
    waitFutureAtMost(f, timeout);
  }
  waitFutureAtMost(statusTask, timeout);
}

}  // namespace pfd::app
