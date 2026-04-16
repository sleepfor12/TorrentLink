#ifndef PFD_BASE_RING_BUFFER_H
#define PFD_BASE_RING_BUFFER_H

#include <atomic>
#include <chrono>
#include <cstddef>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace pfd::base {

constexpr size_t DetermineCacheLineSize() {
#if defined(__x86_64__) || defined(_M_X64)
  return 64;
#elif defined(__aarch64__) || defined(_M_ARM64)
  return 64;
#elif defined(__s390x__)
  return 256;
#else
  return 64;
#endif
}

constexpr size_t kCacheLineSize = DetermineCacheLineSize();

/// @brief 无锁多生产者单消费者环形缓冲区（MPSC）
///
/// 线程安全契约：
/// - 多个生产者线程可并发调用 `Push(...)`。
/// - 仅允许单个消费者线程调用 `TryPop/.../Peek.../Size/Empty`。
/// - 满载时 `Push` 返回 false，不覆盖旧数据。
///
/// 备注：
/// - 实现参考有界队列的 sequence/slot 技巧；`Peek*` 的正确性依赖于
///   单消费者约束。
template <typename T, size_t Capacity> class RingBuffer {
  static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
  static_assert(Capacity > 0, "Capacity must be greater than 0");

public:
  static constexpr size_t capacity() noexcept {
    return Capacity;
  }

  RingBuffer() {
    static_assert(std::is_move_constructible_v<T>, "T must be move-constructible");
    buffer_.reserve(Capacity);
    for (size_t i = 0; i < Capacity; ++i) {
      buffer_.emplace_back(std::nullopt);
    }
    head_.store(0, std::memory_order_relaxed);
    tail_.store(0, std::memory_order_relaxed);
    for (size_t i = 0; i < Capacity; ++i) {
      sequences_[i].store(i, std::memory_order_relaxed);
    }
  }

  RingBuffer(const RingBuffer&) = delete;
  RingBuffer& operator=(const RingBuffer&) = delete;
  RingBuffer(RingBuffer&&) = delete;
  RingBuffer& operator=(RingBuffer&&) = delete;

  bool Push(const T& item) {
    return PushImpl(item);
  }
  bool Push(T&& item) {
    return PushImpl(std::move(item));
  }

  std::optional<T> TryPop() {
    size_t tail = tail_.load(std::memory_order_relaxed);

    while (true) {
      const size_t pos = tail & (Capacity - 1);
      const size_t seq = sequences_[pos].load(std::memory_order_acquire);
      const intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(tail + 1);

      if (diff == 0) {
        // 可读：单消费者不需要 CAS 保护 tail_
        tail_.store(tail + 1, std::memory_order_relaxed);

        auto result = std::move(buffer_[pos]);
        sequences_[pos].store(tail + Capacity, std::memory_order_release);
        return result;
      }

      if (diff < 0) {
        // 为空
        return std::nullopt;
      }

      // 其他生产者正在写入，重试
      tail = tail_.load(std::memory_order_relaxed);
    }
  }

  std::vector<T> TryPopBatch(size_t max_count) {
    std::vector<T> result;
    result.reserve(std::min(max_count, Capacity));
    for (size_t i = 0; i < max_count; ++i) {
      auto item = TryPop();
      if (!item) {
        break;
      }
      result.emplace_back(std::move(*item));
    }
    return result;
  }

  // Peek：非破坏性读取队首元素（单消费者调用语义）
  std::optional<T> PeekAt(size_t index) const {
    const size_t tail = tail_.load(std::memory_order_acquire);
    const size_t head = head_.load(std::memory_order_acquire);

    if (tail > head) {
      return std::nullopt;
    }

    const size_t available = head - tail;
    if (index >= available) {
      return std::nullopt;
    }

    const size_t pos = (tail + index) & (Capacity - 1);
    const size_t seq = sequences_[pos].load(std::memory_order_acquire);
    const intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(tail + index + 1);

    if (diff >= 0) {
      const auto& opt_item = buffer_[pos];
      if (opt_item.has_value()) {
        return opt_item.value();
      }
    }
    return std::nullopt;
  }

  std::optional<T> PeekFront() const {
    return PeekAt(0);
  }

  std::optional<T> PeekLatest() const {
    const size_t tail = tail_.load(std::memory_order_acquire);
    const size_t head = head_.load(std::memory_order_acquire);
    if (tail >= head) {
      return std::nullopt;
    }
    const size_t latest_index = (head - tail) - 1;
    return PeekAt(latest_index);
  }

  // 近似：在 MPSC 里可见元素数量可能瞬间变化
  size_t Size() const noexcept {
    const size_t head = head_.load(std::memory_order_acquire);
    const size_t tail = tail_.load(std::memory_order_acquire);
    return head >= tail ? head - tail : 0;
  }

  bool Empty() const noexcept {
    return Size() == 0;
  }

private:
  template <typename U> bool PushImpl(U&& item) {
    size_t head = head_.load(std::memory_order_relaxed);

    while (true) {
      const size_t pos = head & (Capacity - 1);
      const size_t seq = sequences_[pos].load(std::memory_order_acquire);
      const intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(head);

      if (diff == 0) {
        // 位置可写：竞争 head_ 所有权
        if (head_.compare_exchange_weak(head, head + 1, std::memory_order_relaxed,
                                        std::memory_order_relaxed)) {
          buffer_[pos] = std::make_optional(std::forward<U>(item));
          sequences_[pos].store(head + 1, std::memory_order_release);
          return true;
        }
      } else if (diff < 0) {
        // 满
        return false;
      } else {
        head = head_.load(std::memory_order_relaxed);
      }
    }
  }

  // 数据存储
  std::vector<std::optional<T>> buffer_;

  // 槽位 sequence
  alignas(kCacheLineSize) std::atomic<size_t> sequences_[Capacity];

  // 全局 head/tail（单消费者）
  alignas(kCacheLineSize) std::atomic<size_t> head_{0};
  alignas(kCacheLineSize) std::atomic<size_t> tail_{0};
};

}  // namespace pfd::base

#endif  // PFD_BASE_RING_BUFFER_H
