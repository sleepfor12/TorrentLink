#ifndef PFD_CORE_TASK_PIPELINE_SERVICE_H
#define PFD_CORE_TASK_PIPELINE_SERVICE_H

#include <mutex>
#include <optional>
#include <vector>

#include "core/task_pipeline.h"

namespace pfd::core {

/// @brief 线程安全包装：允许 lt 线程写事件、ui 线程读快照并行协作
class TaskPipelineService {
public:
  void consume(const TaskEvent& event);
  void consumeBatch(const std::vector<TaskEvent>& events);

  [[nodiscard]] std::optional<TaskSnapshot> snapshot(const pfd::base::TaskId& taskId) const;
  [[nodiscard]] std::vector<TaskSnapshot> snapshots() const;

private:
  mutable std::mutex mu_;
  TaskPipeline pipeline_;
};

}  // namespace pfd::core

#endif  // PFD_CORE_TASK_PIPELINE_SERVICE_H
