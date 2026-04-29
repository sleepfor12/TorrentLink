#ifndef PFD_APP_EXIT_COORDINATOR_H
#define PFD_APP_EXIT_COORDINATOR_H

#include <QtCore/QObject>

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <vector>

class QApplication;

namespace pfd::app {

class ExitCoordinator {
public:
  void beginShutdown(std::atomic<bool>& shuttingDown) const;
  void wireAboutToQuit(QApplication* app, QObject* context,
                       const std::function<void()>& fastPersistFn) const;
  void waitFutureAtMost(std::future<void>& fut, std::chrono::milliseconds timeout) const;
  void waitBackgroundTasks(std::vector<std::future<void>>& tasks, std::future<void>& statusTask,
                           std::chrono::milliseconds timeout) const;
};

}  // namespace pfd::app

#endif  // PFD_APP_EXIT_COORDINATOR_H
