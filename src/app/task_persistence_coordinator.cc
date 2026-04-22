#include "app/task_persistence_coordinator.h"

#include <QTimer>

namespace pfd::app {

TaskPersistenceCoordinator::TaskPersistenceCoordinator(QObject* owner, SaveTasksFn saveTasksFn,
                                                       SaveResumeDataFn saveResumeDataFn)
    : owner_(owner), saveTasksFn_(std::move(saveTasksFn)),
      saveResumeDataFn_(std::move(saveResumeDataFn)) {}

void TaskPersistenceCoordinator::setAutoSaveIntervalMs(int intervalMs) {
  intervalMs_ = intervalMs;
  if (autoSaveTimer_ != nullptr) {
    autoSaveTimer_->setInterval(intervalMs_);
  }
}

void TaskPersistenceCoordinator::startAutoSave() {
  if (autoSaveTimer_ == nullptr) {
    autoSaveTimer_ = new QTimer(owner_);
    QObject::connect(autoSaveTimer_, &QTimer::timeout, owner_, [this]() { saveNow(); });
  }
  autoSaveTimer_->setInterval(intervalMs_);
  autoSaveTimer_->start();
}

void TaskPersistenceCoordinator::saveNow() const {
  if (saveTasksFn_) {
    saveTasksFn_();
  }
  if (saveResumeDataFn_) {
    saveResumeDataFn_();
  }
}

void TaskPersistenceCoordinator::saveTasksNow() const {
  if (saveTasksFn_) {
    saveTasksFn_();
  }
}

}  // namespace pfd::app
