#include "core/log_store.h"

#include "base/time_stamps.h"

namespace pfd::core {

LogEntry makeLogEntry(const pfd::base::TaskId& taskId, pfd::base::LogLevel level, QString message,
                      QString detail) {
  LogEntry e;
  e.taskId = taskId;
  e.level = level;
  e.timestampMs = pfd::base::currentMs();
  e.message = std::move(message);
  e.detail = std::move(detail);
  return e;
}

}  // namespace pfd::core
