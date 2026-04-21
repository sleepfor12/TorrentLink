#ifndef PFD_APP_TASK_DETAIL_QUERY_USE_CASE_H
#define PFD_APP_TASK_DETAIL_QUERY_USE_CASE_H

#include "core/task_pipeline_service.h"
#include "lt/session_worker.h"
#include "ui/main_window.h"

namespace pfd::app {

class TaskDetailQueryUseCase {
public:
  TaskDetailQueryUseCase(pfd::core::TaskPipelineService* pipeline, pfd::lt::SessionWorker* worker);

  [[nodiscard]] pfd::ui::MainWindow::CopyPayload queryCopyPayload(
      const pfd::base::TaskId& taskId) const;
  [[nodiscard]] std::vector<pfd::core::TaskFileEntryDto> queryTaskFiles(
      const pfd::base::TaskId& taskId) const;
  [[nodiscard]] pfd::core::TaskTrackerSnapshotDto queryTaskTrackers(
      const pfd::base::TaskId& taskId) const;
  [[nodiscard]] std::vector<pfd::core::TaskPeerDto> queryTaskPeers(
      const pfd::base::TaskId& taskId) const;
  [[nodiscard]] std::vector<pfd::core::TaskWebSeedDto> queryTaskWebSeeds(
      const pfd::base::TaskId& taskId) const;

private:
  pfd::core::TaskPipelineService* pipeline_{nullptr};
  pfd::lt::SessionWorker* worker_{nullptr};
};

}  // namespace pfd::app

#endif  // PFD_APP_TASK_DETAIL_QUERY_USE_CASE_H
