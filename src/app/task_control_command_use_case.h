#ifndef PFD_APP_TASK_CONTROL_COMMAND_USE_CASE_H
#define PFD_APP_TASK_CONTROL_COMMAND_USE_CASE_H

#include <QtCore/QString>

#include <functional>

#include "base/types.h"

namespace pfd::app {

class TaskControlCommandUseCase {
public:
  using PauseFn = std::function<void(const pfd::base::TaskId&)>;
  using ResumeFn = std::function<void(const pfd::base::TaskId&)>;
  using RemoveFn = std::function<void(const pfd::base::TaskId&, bool)>;
  using SetConnectionsFn = std::function<void(const pfd::base::TaskId&, int)>;
  using ForceStartFn = std::function<void(const pfd::base::TaskId&)>;
  using ForceRecheckFn = std::function<void(const pfd::base::TaskId&)>;
  using ForceReannounceFn = std::function<void(const pfd::base::TaskId&)>;
  using SetSequentialFn = std::function<void(const pfd::base::TaskId&, bool)>;
  using SetAutoManagedFn = std::function<void(const pfd::base::TaskId&, bool)>;
  using LogFn = std::function<void(const QString&)>;

  TaskControlCommandUseCase(PauseFn pauseFn, ResumeFn resumeFn, RemoveFn removeFn,
                            SetConnectionsFn setConnectionsFn, ForceStartFn forceStartFn,
                            ForceRecheckFn forceRecheckFn, ForceReannounceFn forceReannounceFn,
                            SetSequentialFn setSequentialFn, SetAutoManagedFn setAutoManagedFn,
                            LogFn logFn);

  void pauseTask(const pfd::base::TaskId& taskId) const;
  void resumeTask(const pfd::base::TaskId& taskId) const;
  void removeTask(const pfd::base::TaskId& taskId, bool removeFiles) const;
  void setTaskConnectionsLimit(const pfd::base::TaskId& taskId, int limit) const;
  void forceStartTask(const pfd::base::TaskId& taskId) const;
  void forceRecheckTask(const pfd::base::TaskId& taskId) const;
  void forceReannounceTask(const pfd::base::TaskId& taskId) const;
  void setSequentialDownload(const pfd::base::TaskId& taskId, bool enabled) const;
  void setAutoManagedTask(const pfd::base::TaskId& taskId, bool enabled) const;

private:
  PauseFn pauseFn_;
  ResumeFn resumeFn_;
  RemoveFn removeFn_;
  SetConnectionsFn setConnectionsFn_;
  ForceStartFn forceStartFn_;
  ForceRecheckFn forceRecheckFn_;
  ForceReannounceFn forceReannounceFn_;
  SetSequentialFn setSequentialFn_;
  SetAutoManagedFn setAutoManagedFn_;
  LogFn logFn_;
};

}  // namespace pfd::app

#endif  // PFD_APP_TASK_CONTROL_COMMAND_USE_CASE_H
