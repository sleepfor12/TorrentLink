#include "app/task_persistence_coordinator.h"

#include <QTimer>

#include <algorithm>

namespace pfd::app {

int TaskPersistenceCoordinator::resumeIntervalFromTaskIntervalMs(int taskIntervalMs) {
  const int t = std::max(2000, taskIntervalMs);
  return std::clamp(t * 10, 30000, 300000);
}

TaskPersistenceCoordinator::TaskPersistenceCoordinator(QObject* owner, SaveTasksFn saveTasksFn,
                                                       SaveResumeDataFn saveResumeDataFn)
    : owner_(owner), saveTasksFn_(std::move(saveTasksFn)),
      saveResumeDataFn_(std::move(saveResumeDataFn)) {}

void TaskPersistenceCoordinator::setAutoSaveIntervalMs(int intervalMs) {
  intervalMs_ = std::max(2000, intervalMs);
  if (taskSaveTimer_ != nullptr) {
    taskSaveTimer_->setInterval(intervalMs_);
  }
  if (resumeSaveTimer_ != nullptr && saveResumeDataFn_) {
    resumeSaveTimer_->setInterval(resumeIntervalFromTaskIntervalMs(intervalMs_));
  }
}

void TaskPersistenceCoordinator::startAutoSave() {
  if (taskSaveTimer_ == nullptr) {
    taskSaveTimer_ = new QTimer(owner_);
    QObject::connect(taskSaveTimer_, &QTimer::timeout, owner_, [this]() {
      if (saveTasksFn_) {
        saveTasksFn_();
      }
    });
  }
  taskSaveTimer_->setInterval(intervalMs_);
  taskSaveTimer_->start();

  if (saveResumeDataFn_) {
    if (resumeSaveTimer_ == nullptr) {
      resumeSaveTimer_ = new QTimer(owner_);
      QObject::connect(resumeSaveTimer_, &QTimer::timeout, owner_, [this]() {
        if (saveResumeDataFn_) {
          saveResumeDataFn_();
        }
      });
    }
    resumeSaveTimer_->setInterval(resumeIntervalFromTaskIntervalMs(intervalMs_));
    resumeSaveTimer_->start();
  }
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
