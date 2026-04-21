#include "app/task_detail_query_use_case.h"

#include <QUuid>

#include "app/task_query_mapper.h"

namespace pfd::app {

TaskDetailQueryUseCase::TaskDetailQueryUseCase(pfd::core::TaskPipelineService* pipeline,
                                               pfd::lt::SessionWorker* worker)
    : pipeline_(pipeline), worker_(worker) {}

pfd::ui::MainWindow::CopyPayload
TaskDetailQueryUseCase::queryCopyPayload(const pfd::base::TaskId& taskId) const {
  pfd::ui::MainWindow::CopyPayload ui;
  if (worker_ == nullptr) {
    return ui;
  }
  const auto payload = worker_->queryTaskCopyPayload(taskId);
  if (pipeline_ != nullptr) {
    const auto snap = pipeline_->snapshot(taskId);
    ui.name = (snap.has_value() ? snap->name : QString());
  }
  ui.infoHashV1 = payload.infoHashV1;
  ui.infoHashV2 = payload.infoHashV2;
  ui.magnet = payload.magnet;
  ui.torrentId =
      payload.torrentId.isEmpty() ? taskId.toString(QUuid::WithoutBraces) : payload.torrentId;
  ui.comment = payload.comment;
  return ui;
}

std::vector<pfd::core::TaskFileEntryDto>
TaskDetailQueryUseCase::queryTaskFiles(const pfd::base::TaskId& taskId) const {
  if (worker_ == nullptr) {
    return {};
  }
  return pfd::app::mapTaskFiles(worker_->queryTaskFiles(taskId));
}

pfd::core::TaskTrackerSnapshotDto
TaskDetailQueryUseCase::queryTaskTrackers(const pfd::base::TaskId& taskId) const {
  if (worker_ == nullptr) {
    return {};
  }
  return pfd::app::mapTaskTrackers(worker_->queryTaskTrackers(taskId));
}

std::vector<pfd::core::TaskPeerDto>
TaskDetailQueryUseCase::queryTaskPeers(const pfd::base::TaskId& taskId) const {
  if (worker_ == nullptr) {
    return {};
  }
  return pfd::app::mapTaskPeers(worker_->queryTaskPeers(taskId));
}

std::vector<pfd::core::TaskWebSeedDto>
TaskDetailQueryUseCase::queryTaskWebSeeds(const pfd::base::TaskId& taskId) const {
  if (worker_ == nullptr) {
    return {};
  }
  return pfd::app::mapTaskWebSeeds(worker_->queryTaskWebSeeds(taskId));
}

}  // namespace pfd::app
