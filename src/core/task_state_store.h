#ifndef PFD_CORE_TASK_STATE_STORE_H
#define PFD_CORE_TASK_STATE_STORE_H

#include <QtCore/qhashfunctions.h>

#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "core/task_event.h"
#include "core/task_snapshot.h"

namespace pfd::core {

/// @brief 将 TaskEvent 聚合为 TaskSnapshot（线程安全）
class TaskStateStore {
public:
  void applyEvent(const TaskEvent& event);

  [[nodiscard]] std::optional<TaskSnapshot> snapshot(const pfd::base::TaskId& taskId) const;
  [[nodiscard]] std::vector<TaskSnapshot> snapshots() const;

private:
  struct TaskIdHash {
    size_t operator()(const pfd::base::TaskId& id) const noexcept {
      return qHash(id);
    }
  };

  mutable std::mutex mu_;
  std::unordered_map<pfd::base::TaskId, TaskSnapshot, TaskIdHash> tasks_;
};

}  // namespace pfd::core

#endif  // PFD_CORE_TASK_STATE_STORE_H
