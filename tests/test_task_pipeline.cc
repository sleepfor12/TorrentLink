#include <QtCore/QUuid>

#include <gtest/gtest.h>

#include "core/task_pipeline.h"

namespace {

TEST(TaskPipeline, ConsumeEventUpdatesSnapshot) {
  pfd::core::TaskPipeline pipeline;
  const auto id = QUuid::createUuid();

  pfd::core::TaskEvent e;
  e.type = pfd::core::TaskEventType::kProgressUpdated;
  e.taskId = id;
  e.totalBytes = 1000;
  e.downloadedBytes = 300;
  pipeline.consume(e);

  auto snap = pipeline.snapshot(id);
  ASSERT_TRUE(snap.has_value());
  EXPECT_EQ(snap->totalBytes, 1000);
  EXPECT_EQ(snap->downloadedBytes, 300);
}

TEST(TaskPipeline, ConsumeBatchUpdatesMultipleSnapshots) {
  pfd::core::TaskPipeline pipeline;
  const auto id1 = QUuid::createUuid();
  const auto id2 = QUuid::createUuid();

  pfd::core::TaskEvent e1;
  e1.type = pfd::core::TaskEventType::kUpsertMeta;
  e1.taskId = id1;
  e1.name = QStringLiteral("a.iso");

  pfd::core::TaskEvent e2;
  e2.type = pfd::core::TaskEventType::kProgressUpdated;
  e2.taskId = id2;
  e2.totalBytes = 100;
  e2.downloadedBytes = 60;

  pipeline.consumeBatch({e1, e2});

  const auto s1 = pipeline.snapshot(id1);
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(s1->name, QStringLiteral("a.iso"));

  const auto s2 = pipeline.snapshot(id2);
  ASSERT_TRUE(s2.has_value());
  EXPECT_EQ(s2->downloadedBytes, 60);
}

}  // namespace
