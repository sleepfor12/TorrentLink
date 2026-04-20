#ifndef PFD_APP_TASK_BATCH_USE_CASE_H
#define PFD_APP_TASK_BATCH_USE_CASE_H

#include <functional>
#include <vector>

#include "core/task_snapshot.h"
#include "lt/session_worker.h"

namespace pfd::app {

class TaskBatchUseCase {
public:
  using SnapshotProvider = std::function<std::vector<pfd::core::TaskSnapshot>()>;

  TaskBatchUseCase(pfd::lt::SessionWorker* worker, SnapshotProvider snapshotsProvider);

  void pauseAllActiveTasks() const;
  void resumeAllPausedTasks() const;
  void pauseByIds(const std::vector<pfd::base::TaskId>& taskIds) const;
  void resumeByIds(const std::vector<pfd::base::TaskId>& taskIds) const;

private:
  pfd::lt::SessionWorker* worker_{nullptr};
  SnapshotProvider snapshotsProvider_;
};

}  // namespace pfd::app

#endif  // PFD_APP_TASK_BATCH_USE_CASE_H
