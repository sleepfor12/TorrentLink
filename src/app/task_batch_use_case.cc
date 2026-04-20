#include "app/task_batch_use_case.h"

#include "base/types.h"

namespace pfd::app {

TaskBatchUseCase::TaskBatchUseCase(pfd::lt::SessionWorker* worker,
                                   SnapshotProvider snapshotsProvider)
    : worker_(worker), snapshotsProvider_(std::move(snapshotsProvider)) {}

void TaskBatchUseCase::pauseAllActiveTasks() const {
  if (worker_ == nullptr || !snapshotsProvider_) {
    return;
  }
  const auto snapshots = snapshotsProvider_();
  for (const auto& s : snapshots) {
    if (s.status == pfd::base::TaskStatus::kDownloading ||
        s.status == pfd::base::TaskStatus::kSeeding) {
      worker_->pauseTask(s.taskId);
    }
  }
}

void TaskBatchUseCase::resumeAllPausedTasks() const {
  if (worker_ == nullptr || !snapshotsProvider_) {
    return;
  }
  const auto snapshots = snapshotsProvider_();
  for (const auto& s : snapshots) {
    if (s.status == pfd::base::TaskStatus::kPaused) {
      worker_->resumeTask(s.taskId);
    }
  }
}

void TaskBatchUseCase::pauseByIds(const std::vector<pfd::base::TaskId>& taskIds) const {
  if (worker_ == nullptr) {
    return;
  }
  for (const auto& taskId : taskIds) {
    worker_->pauseTask(taskId);
  }
}

void TaskBatchUseCase::resumeByIds(const std::vector<pfd::base::TaskId>& taskIds) const {
  if (worker_ == nullptr) {
    return;
  }
  for (const auto& taskId : taskIds) {
    worker_->resumeTask(taskId);
  }
}

}  // namespace pfd::app
