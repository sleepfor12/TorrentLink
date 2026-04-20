#include "app/event_ingest_orchestrator.h"

#include <QDateTime>
#include <QUuid>

namespace pfd::app {

EventIngestOrchestrator::EventIngestOrchestrator(pfd::core::TaskPipelineService* pipeline)
    : pipeline_(pipeline) {}

void EventIngestOrchestrator::lockProgressStatus(const pfd::base::TaskId& taskId,
                                                 qint64 lockUntilMs) {
  statusLockUntilMs_[taskId.toString(QUuid::WithoutBraces)] = lockUntilMs;
}

EventIngestOrchestrator::IngestResult
EventIngestOrchestrator::ingest(const std::vector<pfd::lt::LtAlertView>& views) {
  IngestResult result;
  if (pipeline_ == nullptr || views.empty()) {
    return result;
  }

  std::vector<pfd::core::TaskEvent> events;
  events.reserve(views.size());
  for (const auto& view : views) {
    const auto ev = pfd::lt::mapAlertToTaskEvent(view);
    if (ev.has_value()) {
      events.push_back(*ev);
    }
  }
  if (events.empty()) {
    return result;
  }

  const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
  for (auto& ev : events) {
    if (ev.type == pfd::core::TaskEventType::kProgressUpdated) {
      const QString key = ev.taskId.toString(QUuid::WithoutBraces);
      auto lockIt = statusLockUntilMs_.find(key);
      if (lockIt != statusLockUntilMs_.end()) {
        if (nowMs < lockIt->second) {
          ev.status = pfd::base::TaskStatus::kUnknown;
        } else {
          statusLockUntilMs_.erase(lockIt);
        }
      }
    }
  }

  pipeline_->consumeBatch(events);

  for (const auto& ev : events) {
    if (ev.type == pfd::core::TaskEventType::kRemoved) {
      result.removedTaskIds.push_back(ev.taskId);
    } else if (ev.type == pfd::core::TaskEventType::kDuplicateRejected) {
      result.duplicateRejectedTaskIds.push_back(ev.taskId);
    }
  }

  return result;
}

}  // namespace pfd::app
