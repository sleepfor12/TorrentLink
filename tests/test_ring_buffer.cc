#include <atomic>
#include <gtest/gtest.h>
#include <optional>
#include <thread>
#include <unordered_set>
#include <vector>

#include "base/ring_buffer.h"

namespace {

using pfd::base::RingBuffer;

TEST(RingBuffer, EmptyTryPopReturnsNullopt) {
  RingBuffer<int, 8> rb;
  EXPECT_TRUE(rb.Empty());
  EXPECT_FALSE(rb.TryPop().has_value());
  EXPECT_FALSE(rb.PeekFront().has_value());
}

TEST(RingBuffer, PushPopFifoAndPeekSemantics) {
  RingBuffer<int, 8> rb;
  for (int i = 0; i < 8; ++i) {
    EXPECT_TRUE(rb.Push(i));
  }
  EXPECT_FALSE(rb.Push(8));  // full

  // peek does not pop
  auto p0 = rb.PeekAt(0);
  ASSERT_TRUE(p0.has_value());
  EXPECT_EQ(*p0, 0);
  auto pLatest = rb.PeekLatest();
  ASSERT_TRUE(pLatest.has_value());
  EXPECT_EQ(*pLatest, 7);

  for (int i = 0; i < 8; ++i) {
    auto v = rb.TryPop();
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, i);
  }
  EXPECT_TRUE(rb.Empty());
  EXPECT_FALSE(rb.TryPop().has_value());
}

TEST(RingBuffer, PushAfterDrainSucceeds) {
  RingBuffer<int, 4> rb;
  EXPECT_TRUE(rb.Push(1));
  EXPECT_TRUE(rb.Push(2));

  for (int i = 0; i < 2; ++i) {
    auto v = rb.TryPop();
    ASSERT_TRUE(v.has_value());
    (void)*v;
  }
  EXPECT_TRUE(rb.Empty());
  EXPECT_TRUE(rb.Push(3));
  EXPECT_EQ(*rb.TryPop(), 3);
}

TEST(RingBuffer, MpscMultiProducerSingleConsumerNoLoss) {
  constexpr size_t kProducers = 4;
  constexpr size_t kPerProducer = 1000;
  constexpr size_t expected = kProducers * kPerProducer;
  constexpr size_t cap = 4096;  // power of two >= expected
  static_assert(cap >= expected, "capacity should be large enough");

  RingBuffer<int, cap> rb;

  std::atomic<size_t> pushed_total{0};

  std::vector<std::thread> producers;
  producers.reserve(kProducers);
  for (size_t pid = 0; pid < kProducers; ++pid) {
    producers.emplace_back([&rb, &pushed_total, pid]() {
      for (size_t seq = 0; seq < kPerProducer; ++seq) {
        const int value = static_cast<int>(pid * 1000000 + seq);
        while (!rb.Push(value)) {
          // Should not happen (cap >= expected), but keep stable anyway.
          std::this_thread::yield();
        }
        pushed_total.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  // Single consumer
  std::unordered_set<int> popped;
  popped.reserve(expected);

  while (popped.size() < expected) {
    auto item = rb.TryPop();
    if (item.has_value()) {
      popped.insert(*item);
    } else {
      std::this_thread::yield();
    }
  }

  for (auto& t : producers) {
    t.join();
  }

  EXPECT_EQ(popped.size(), expected);
  EXPECT_EQ(pushed_total.load(std::memory_order_relaxed), expected);
}

}  // namespace
