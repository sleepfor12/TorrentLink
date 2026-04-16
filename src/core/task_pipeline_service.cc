#include "core/task_pipeline_service.h"

#include "core/logger.h"

namespace pfd::core {

void TaskPipelineService::consume(const TaskEvent& event) {
  std::lock_guard<std::mutex> g(mu_);
  pipeline_.consume(event);
}

void TaskPipelineService::consumeBatch(const std::vector<TaskEvent>& events) {
  if (!events.empty()) {
    LOG_DEBUG(QStringLiteral("[pipeline.service] consumeBatch size=%1").arg(events.size()));
  }
  std::lock_guard<std::mutex> g(mu_);
  pipeline_.consumeBatch(events);
}

std::optional<TaskSnapshot> TaskPipelineService::snapshot(const pfd::base::TaskId& taskId) const {
  std::lock_guard<std::mutex> g(mu_);
  return pipeline_.snapshot(taskId);
}

std::vector<TaskSnapshot> TaskPipelineService::snapshots() const {
  std::lock_guard<std::mutex> g(mu_);
  return pipeline_.snapshots();
}

}  // namespace pfd::core
