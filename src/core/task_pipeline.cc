#include "core/task_pipeline.h"

#include "core/logger.h"

namespace pfd::core {

namespace {

const char* eventTypeToText(TaskEventType t) {
  switch (t) {
    case TaskEventType::kUpsertMeta:
      return "upsert_meta";
    case TaskEventType::kStatusChanged:
      return "status_changed";
    case TaskEventType::kProgressUpdated:
      return "progress_updated";
    case TaskEventType::kRateUpdated:
      return "rate_updated";
    case TaskEventType::kErrorOccurred:
      return "error_occurred";
    case TaskEventType::kDuplicateRejected:
      return "duplicate_rejected";
    case TaskEventType::kRemoved:
      return "removed";
  }
  return "unknown";
}

}  // namespace

void TaskPipeline::consume(const TaskEvent& event) {
  store_.applyEvent(event);
  LOG_DEBUG(QStringLiteral("task_event consumed: type=%1")
                .arg(QString::fromUtf8(eventTypeToText(event.type))));
}

void TaskPipeline::consumeBatch(const std::vector<TaskEvent>& events) {
  for (const auto& event : events) {
    store_.applyEvent(event);
  }
  LOG_DEBUG(QStringLiteral("task_events consumed in batch: count=%1")
                .arg(static_cast<int>(events.size())));
}

std::optional<TaskSnapshot> TaskPipeline::snapshot(const pfd::base::TaskId& taskId) const {
  return store_.snapshot(taskId);
}

std::vector<TaskSnapshot> TaskPipeline::snapshots() const {
  return store_.snapshots();
}

}  // namespace pfd::core
