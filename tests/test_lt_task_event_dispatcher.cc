#include <QtCore/QUuid>

#include <gtest/gtest.h>

#include "lt/task_event_dispatcher.h"

namespace {

TEST(LtTaskEventDispatcher, DispatchesMappedEventToPipeline) {
  pfd::core::TaskPipeline pipeline;
  pfd::lt::LtAlertView alert;
  alert.type = pfd::lt::LtAlertView::Type::kTaskProgress;
  alert.taskId = QUuid::createUuid();
  alert.totalBytes = 2000;
  alert.downloadedBytes = 500;

  pfd::lt::dispatchAlertViewToPipeline(alert, &pipeline);

  auto snap = pipeline.snapshot(alert.taskId);
  ASSERT_TRUE(snap.has_value());
  EXPECT_EQ(snap->totalBytes, 2000);
  EXPECT_EQ(snap->downloadedBytes, 500);
}

TEST(LtTaskEventDispatcher, NullPipelineNoCrash) {
  pfd::lt::LtAlertView alert;
  alert.type = pfd::lt::LtAlertView::Type::kTaskError;
  alert.taskId = QUuid::createUuid();
  alert.message = QStringLiteral("x");
  pfd::lt::dispatchAlertViewToPipeline(alert, nullptr);
  SUCCEED();
}

TEST(LtTaskEventDispatcher, DispatchBatchToPipeline) {
  pfd::core::TaskPipeline pipeline;
  const auto id1 = QUuid::createUuid();
  const auto id2 = QUuid::createUuid();

  pfd::lt::LtAlertView a1;
  a1.type = pfd::lt::LtAlertView::Type::kTaskAdded;
  a1.taskId = id1;
  a1.name = QStringLiteral("a.iso");

  pfd::lt::LtAlertView a2;
  a2.type = pfd::lt::LtAlertView::Type::kTaskProgress;
  a2.taskId = id2;
  a2.totalBytes = 1000;
  a2.downloadedBytes = 500;

  pfd::lt::dispatchAlertViewsToPipeline({a1, a2}, &pipeline);

  const auto s1 = pipeline.snapshot(id1);
  ASSERT_TRUE(s1.has_value());
  EXPECT_EQ(s1->name, QStringLiteral("a.iso"));

  const auto s2 = pipeline.snapshot(id2);
  ASSERT_TRUE(s2.has_value());
  EXPECT_EQ(s2->downloadedBytes, 500);
}

}  // namespace
