#ifndef PFD_APP_TASK_PERSISTENCE_COORDINATOR_H
#define PFD_APP_TASK_PERSISTENCE_COORDINATOR_H

#include <functional>

class QObject;
class QTimer;

namespace pfd::app {

class TaskPersistenceCoordinator {
public:
  using SaveTasksFn = std::function<void()>;
  using SaveResumeDataFn = std::function<void()>;

  TaskPersistenceCoordinator(QObject* owner, SaveTasksFn saveTasksFn,
                             SaveResumeDataFn saveResumeDataFn);

  void setAutoSaveIntervalMs(int intervalMs);
  void startAutoSave();
  void saveNow() const;
  void saveTasksNow() const;

private:
  QObject* owner_{nullptr};
  QTimer* autoSaveTimer_{nullptr};
  int intervalMs_{5000};
  SaveTasksFn saveTasksFn_;
  SaveResumeDataFn saveResumeDataFn_;
};

}  // namespace pfd::app

#endif  // PFD_APP_TASK_PERSISTENCE_COORDINATOR_H
