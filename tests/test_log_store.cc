#include <QtCore/QUuid>

#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <unordered_set>
#include <vector>

#include "core/log_store.h"

namespace {

using pfd::base::LogLevel;
using pfd::base::TaskId;
using pfd::core::LogStoreT;

TEST(LogStore, SnapshotOrdersFifo) {
  LogStoreT<8> store;
  const TaskId taskId = QUuid::createUuid();

  store.append(taskId, LogLevel::kInfo, QStringLiteral("m0"));
  store.append(taskId, LogLevel::kInfo, QStringLiteral("m1"));
  store.append(taskId, LogLevel::kInfo, QStringLiteral("m2"));

  const auto logs = store.snapshot(taskId, 10);
  ASSERT_EQ(logs.size(), 3u);
  EXPECT_EQ(logs[0].message, QStringLiteral("m0"));
  EXPECT_EQ(logs[1].message, QStringLiteral("m1"));
  EXPECT_EQ(logs[2].message, QStringLiteral("m2"));
}

TEST(LogStore, DroppedCountInNoOverwriteModeWhenFull) {
  LogStoreT<4> store;
  const TaskId taskId = QUuid::createUuid();

  // 满队（容量=4），后续写入会失败且不覆盖。
  for (int i = 0; i < 6; ++i) {
    store.append(taskId, LogLevel::kInfo, QString::number(i));
  }

  const auto logs = store.snapshot(taskId, 100);
  ASSERT_EQ(logs.size(), 4u);

  // 保留最早的 4 条：0..3
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(logs[static_cast<size_t>(i)].message, QString::number(i));
  }

  EXPECT_EQ(store.droppedCount(taskId), 2u);
}

TEST(LogStore, MpscMultiProducerSnapshotNoLossWhenCapacityEnough) {
  constexpr size_t kCapacity = 4096;  // power of two
  constexpr size_t kProducers = 4;
  constexpr size_t kPerProducer = 1000;
  constexpr size_t kExpected = kProducers * kPerProducer;
  static_assert(kCapacity >= kExpected, "capacity should be large enough");

  LogStoreT<kCapacity> store;
  const TaskId taskId = QUuid::createUuid();

  std::atomic<size_t> ready{0};
  std::vector<std::thread> producers;
  producers.reserve(kProducers);

  // 记录期望的整数集合（message 中存储该整数）
  std::unordered_set<long long> expected;
  expected.reserve(kExpected);
  for (size_t pid = 0; pid < kProducers; ++pid) {
    for (size_t seq = 0; seq < kPerProducer; ++seq) {
      expected.insert(static_cast<long long>(pid) * 1000000LL + static_cast<long long>(seq));
    }
  }

  for (size_t pid = 0; pid < kProducers; ++pid) {
    producers.emplace_back([&store, &taskId, pid, &ready]() {
      ready.fetch_add(1, std::memory_order_relaxed);
      // 仅依赖 join 后快照；不做严格同步屏障以保持测试简单。
      for (size_t seq = 0; seq < kPerProducer; ++seq) {
        const long long value =
            static_cast<long long>(pid) * 1000000LL + static_cast<long long>(seq);
        store.append(taskId, LogLevel::kInfo, QString::number(value));
      }
    });
  }

  for (auto& t : producers) {
    t.join();
  }

  const auto logs = store.snapshot(taskId, kExpected);
  ASSERT_EQ(logs.size(), kExpected);

  std::unordered_set<long long> actual;
  actual.reserve(kExpected);
  for (const auto& e : logs) {
    bool ok = false;
    const long long v = e.message.toLongLong(&ok);
    ASSERT_TRUE(ok);
    actual.insert(v);
  }

  EXPECT_EQ(actual, expected);
}

}  // namespace
