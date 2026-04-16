#ifndef PFD_CORE_TASK_PIPELINE_H
#define PFD_CORE_TASK_PIPELINE_H

#include <optional>
#include <vector>

#include "core/task_event.h"
#include "core/task_state_store.h"

namespace pfd::core {

/// @brief Core 主链路入口：消费 TaskEvent，产出 TaskSnapshot
class TaskPipeline {
public:
  void consume(const TaskEvent& event);
  void consumeBatch(const std::vector<TaskEvent>& events);

  [[nodiscard]] std::optional<TaskSnapshot> snapshot(const pfd::base::TaskId& taskId) const;
  [[nodiscard]] std::vector<TaskSnapshot> snapshots() const;

private:
  TaskStateStore store_;
};

}  // namespace pfd::core

#endif  // PFD_CORE_TASK_PIPELINE_H
