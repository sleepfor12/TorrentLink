#ifndef PFD_APP_EVENT_INGEST_ORCHESTRATOR_H
#define PFD_APP_EVENT_INGEST_ORCHESTRATOR_H

#include <QtCore/QString>

#include <functional>
#include <unordered_map>
#include <vector>

#include "core/task_pipeline_service.h"
#include "lt/task_event_mapper.h"

namespace pfd::app {

class EventIngestOrchestrator {
public:
  struct IngestResult {
    std::vector<pfd::base::TaskId> duplicateRejectedTaskIds;
    std::vector<pfd::base::TaskId> removedTaskIds;
  };

  explicit EventIngestOrchestrator(pfd::core::TaskPipelineService* pipeline);

  void lockProgressStatus(const pfd::base::TaskId& taskId, qint64 lockUntilMs);
  IngestResult ingest(const std::vector<pfd::lt::LtAlertView>& views);

private:
  pfd::core::TaskPipelineService* pipeline_{nullptr};
  std::unordered_map<QString, qint64> statusLockUntilMs_;
};

}  // namespace pfd::app

#endif  // PFD_APP_EVENT_INGEST_ORCHESTRATOR_H
