#include <QtCore/QUuid>

#include <gtest/gtest.h>

#include "core/task_pipeline_service.h"

namespace {

TEST(TaskPipelineService, ThreadSafeConsumeAndQuery) {
  pfd::core::TaskPipelineService svc;
  const auto id = QUuid::createUuid();

  pfd::core::TaskEvent e;
  e.type = pfd::core::TaskEventType::kUpsertMeta;
  e.taskId = id;
  e.name = QStringLiteral("a.iso");
  svc.consume(e);

  auto s = svc.snapshot(id);
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->name, QStringLiteral("a.iso"));
}

TEST(TaskPipelineService, ConsumeBatchAndQuery) {
  pfd::core::TaskPipelineService svc;
  const auto id = QUuid::createUuid();

  pfd::core::TaskEvent e1;
  e1.type = pfd::core::TaskEventType::kUpsertMeta;
  e1.taskId = id;
  e1.name = QStringLiteral("batch.iso");

  pfd::core::TaskEvent e2;
  e2.type = pfd::core::TaskEventType::kRateUpdated;
  e2.taskId = id;
  e2.downloadRate = 128.0;

  svc.consumeBatch({e1, e2});

  const auto s = svc.snapshot(id);
  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->name, QStringLiteral("batch.iso"));
  EXPECT_DOUBLE_EQ(s->downloadRate, 128.0);
}

}  // namespace
