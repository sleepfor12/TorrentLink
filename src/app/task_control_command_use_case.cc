#include "app/task_control_command_use_case.h"

namespace pfd::app {

TaskControlCommandUseCase::TaskControlCommandUseCase(
    PauseFn pauseFn, ResumeFn resumeFn, RemoveFn removeFn, SetConnectionsFn setConnectionsFn,
    ForceStartFn forceStartFn, ForceRecheckFn forceRecheckFn, ForceReannounceFn forceReannounceFn,
    SetSequentialFn setSequentialFn, SetAutoManagedFn setAutoManagedFn, LogFn logFn)
    : pauseFn_(std::move(pauseFn)), resumeFn_(std::move(resumeFn)), removeFn_(std::move(removeFn)),
      setConnectionsFn_(std::move(setConnectionsFn)), forceStartFn_(std::move(forceStartFn)),
      forceRecheckFn_(std::move(forceRecheckFn)),
      forceReannounceFn_(std::move(forceReannounceFn)),
      setSequentialFn_(std::move(setSequentialFn)),
      setAutoManagedFn_(std::move(setAutoManagedFn)), logFn_(std::move(logFn)) {}

void TaskControlCommandUseCase::pauseTask(const pfd::base::TaskId& taskId) const {
  if (pauseFn_) {
    pauseFn_(taskId);
  }
  if (logFn_) {
    logFn_(QStringLiteral("Pause requested, waiting worker confirmation. taskId=%1")
               .arg(taskId.toString()));
  }
}

void TaskControlCommandUseCase::resumeTask(const pfd::base::TaskId& taskId) const {
  if (resumeFn_) {
    resumeFn_(taskId);
  }
  if (logFn_) {
    logFn_(QStringLiteral("Resume requested, waiting worker confirmation. taskId=%1")
               .arg(taskId.toString()));
  }
}

void TaskControlCommandUseCase::removeTask(const pfd::base::TaskId& taskId, bool removeFiles) const {
  if (removeFn_) {
    removeFn_(taskId, removeFiles);
  }
  if (logFn_) {
    logFn_(QStringLiteral("Remove requested, waiting worker confirmation. taskId=%1 removeFiles=%2")
               .arg(taskId.toString())
               .arg(removeFiles ? QStringLiteral("true") : QStringLiteral("false")));
  }
}

void TaskControlCommandUseCase::setTaskConnectionsLimit(const pfd::base::TaskId& taskId,
                                                        int limit) const {
  if (setConnectionsFn_) {
    setConnectionsFn_(taskId, limit);
  }
  if (logFn_) {
    logFn_(QStringLiteral("Task connections limit updated. taskId=%1 limit=%2")
               .arg(taskId.toString())
               .arg(limit));
  }
}

void TaskControlCommandUseCase::forceStartTask(const pfd::base::TaskId& taskId) const {
  if (forceStartFn_) {
    forceStartFn_(taskId);
  }
  if (logFn_) {
    logFn_(QStringLiteral("Force start requested. taskId=%1").arg(taskId.toString()));
  }
}

void TaskControlCommandUseCase::forceRecheckTask(const pfd::base::TaskId& taskId) const {
  if (forceRecheckFn_) {
    forceRecheckFn_(taskId);
  }
  if (logFn_) {
    logFn_(QStringLiteral("Force recheck requested. taskId=%1").arg(taskId.toString()));
  }
}

void TaskControlCommandUseCase::forceReannounceTask(const pfd::base::TaskId& taskId) const {
  if (forceReannounceFn_) {
    forceReannounceFn_(taskId);
  }
  if (logFn_) {
    logFn_(QStringLiteral("Force reannounce requested. taskId=%1").arg(taskId.toString()));
  }
}

void TaskControlCommandUseCase::setSequentialDownload(const pfd::base::TaskId& taskId,
                                                      bool enabled) const {
  if (setSequentialFn_) {
    setSequentialFn_(taskId, enabled);
  }
  if (logFn_) {
    logFn_(QStringLiteral("Sequential download changed. taskId=%1 enabled=%2")
               .arg(taskId.toString())
               .arg(enabled ? QStringLiteral("true") : QStringLiteral("false")));
  }
}

void TaskControlCommandUseCase::setAutoManagedTask(const pfd::base::TaskId& taskId,
                                                   bool enabled) const {
  if (setAutoManagedFn_) {
    setAutoManagedFn_(taskId, enabled);
  }
  if (logFn_) {
    logFn_(QStringLiteral("Auto managed changed. taskId=%1 enabled=%2")
               .arg(taskId.toString())
               .arg(enabled ? QStringLiteral("true") : QStringLiteral("false")));
  }
}

}  // namespace pfd::app
