#include "lt/task_event_dispatcher.h"

namespace pfd::lt {

void dispatchAlertViewToPipeline(const LtAlertView& alert, pfd::core::TaskPipeline* pipeline) {
  if (pipeline == nullptr) {
    return;
  }
  const auto ev = mapAlertToTaskEvent(alert);
  if (!ev.has_value()) {
    return;
  }
  pipeline->consume(*ev);
}

void dispatchAlertViewsToPipeline(const std::vector<LtAlertView>& alerts,
                                  pfd::core::TaskPipeline* pipeline) {
  if (pipeline == nullptr) {
    return;
  }
  std::vector<pfd::core::TaskEvent> events;
  events.reserve(alerts.size());
  for (const auto& alert : alerts) {
    const auto ev = mapAlertToTaskEvent(alert);
    if (ev.has_value()) {
      events.push_back(*ev);
    }
  }
  if (!events.empty()) {
    pipeline->consumeBatch(events);
  }
}

}  // namespace pfd::lt
