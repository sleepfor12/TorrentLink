#ifndef PFD_CORE_LOG_STORE_H
#define PFD_CORE_LOG_STORE_H

#include <QtCore/QString>
#include <QtCore/QUuid>
#include <QtCore/qhashfunctions.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/ring_buffer.h"
#include "base/types.h"

namespace pfd::core {

struct LogEntry {
  pfd::base::TaskId taskId;
  pfd::base::LogLevel level;
  qint64 timestampMs;
  QString message;
  QString detail;
};

LogEntry makeLogEntry(const pfd::base::TaskId& taskId, pfd::base::LogLevel level, QString message,
                      QString detail = {});

template <size_t Capacity> class LogStoreT {
public:
  static constexpr size_t kCapacity = Capacity;

  static_assert((Capacity & (Capacity - 1)) == 0,
                "LogStore capacity must be a power of two (RingBuffer requirement)");

  void append(LogEntry entry) {
    auto store = getOrCreate(entry.taskId);

    // MPSC push：可能失败（满队且不覆盖）
    const bool ok = store->queue.Push(std::move(entry));
    if (!ok) {
      store->dropped.fetch_add(1, std::memory_order_relaxed);
    }
  }

  void append(const pfd::base::TaskId& taskId, pfd::base::LogLevel level, const QString& message,
              const QString& detail = {}) {
    append(makeLogEntry(taskId, level, message, detail));
  }

  /// @brief 获取任务日志快照（非破坏性，按 FIFO 顺序：旧->新）
  [[nodiscard]] std::vector<LogEntry> snapshot(const pfd::base::TaskId& taskId, size_t max_count) {
    std::lock_guard<std::mutex> guard(consumer_mutex_);

    std::shared_ptr<PerTaskStore> store;
    {
      std::lock_guard<std::mutex> g(stores_mutex_);
      auto it = stores_.find(taskId);
      if (it == stores_.end()) {
        return {};
      }
      store = it->second;
    }

    if (!store) {
      return {};
    }

    const size_t sz = store->queue.Size();
    const size_t count = std::min(max_count, sz);
    std::vector<LogEntry> result;
    result.reserve(count);

    for (size_t i = 0; i < count; ++i) {
      auto v = store->queue.PeekAt(i);
      if (v.has_value()) {
        result.emplace_back(std::move(*v));
      }
    }
    return result;
  }

  [[nodiscard]] uint64_t droppedCount(const pfd::base::TaskId& taskId) const {
    std::lock_guard<std::mutex> guard(stores_mutex_);
    auto it = stores_.find(taskId);
    if (it == stores_.end() || !it->second) {
      return 0;
    }
    return it->second->dropped.load(std::memory_order_relaxed);
  }

  /// @brief 清空任务日志（破坏性：drain pop）
  void clear(const pfd::base::TaskId& taskId) {
    std::lock_guard<std::mutex> guard(consumer_mutex_);

    std::shared_ptr<PerTaskStore> store;
    {
      std::lock_guard<std::mutex> g(stores_mutex_);
      auto it = stores_.find(taskId);
      if (it == stores_.end() || !it->second) {
        return;
      }
      store = it->second;
    }

    if (!store) {
      return;
    }

    while (true) {
      auto item = store->queue.TryPop();
      if (!item.has_value()) {
        break;
      }
    }
    store->dropped.store(0, std::memory_order_relaxed);
  }

private:
  struct PerTaskStore {
    pfd::base::RingBuffer<LogEntry, Capacity> queue;
    std::atomic<uint64_t> dropped{0};
  };

  struct TaskIdHash {
    size_t operator()(const pfd::base::TaskId& id) const noexcept {
      return qHash(id);
    }
  };

  std::shared_ptr<PerTaskStore> getOrCreate(const pfd::base::TaskId& taskId) {
    std::lock_guard<std::mutex> g(stores_mutex_);
    auto it = stores_.find(taskId);
    if (it != stores_.end() && it->second) {
      return it->second;
    }
    auto ptr = std::make_shared<PerTaskStore>();
    stores_.emplace(taskId, ptr);
    return ptr;
  }

  mutable std::mutex stores_mutex_;
  std::mutex consumer_mutex_;
  std::unordered_map<pfd::base::TaskId, std::shared_ptr<PerTaskStore>, TaskIdHash> stores_;
};

// 默认：每任务最多保存 8192 条（不覆盖；满队后新的 push 会失败）
// 说明：`RingBuffer` 需要容量为 2 的幂，因此用 8192 覆盖“约 5000”这个目标。
using LogStore = LogStoreT<8192>;

}  // namespace pfd::core

#endif  // PFD_CORE_LOG_STORE_H
