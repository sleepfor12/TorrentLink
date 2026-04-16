#include <QtCore/QUuid>

#include <gtest/gtest.h>

#include "lt/task_event_mapper.h"

namespace {

TEST(LtTaskEventMapper, ProgressAlertMapsToProgressEvent) {
  pfd::lt::LtAlertView a;
  a.type = pfd::lt::LtAlertView::Type::kTaskProgress;
  a.taskId = QUuid::createUuid();
  a.status = pfd::base::TaskStatus::kDownloading;
  a.totalBytes = 1000;
  a.downloadedBytes = 200;
  a.uploadedBytes = 50;
  a.eventMs = 1234;

  auto ev = pfd::lt::mapAlertToTaskEvent(a);
  ASSERT_TRUE(ev.has_value());
  EXPECT_EQ(ev->type, pfd::core::TaskEventType::kProgressUpdated);
  EXPECT_EQ(ev->status, pfd::base::TaskStatus::kDownloading);
  EXPECT_EQ(ev->totalBytes, 1000);
  EXPECT_EQ(ev->downloadedBytes, 200);
  EXPECT_EQ(ev->uploadedBytes, 50);
  EXPECT_EQ(ev->eventMs, 1234);
}

TEST(LtTaskEventMapper, DuplicateRejectedMapsToDuplicateEvent) {
  pfd::lt::LtAlertView a;
  a.type = pfd::lt::LtAlertView::Type::kDuplicateRejected;
  a.taskId = QUuid::createUuid();
  a.message = QStringLiteral("任务已存在（相同 info-hash），已忽略重复添加");

  auto ev = pfd::lt::mapAlertToTaskEvent(a);
  ASSERT_TRUE(ev.has_value());
  EXPECT_EQ(ev->type, pfd::core::TaskEventType::kDuplicateRejected);
  EXPECT_EQ(ev->errorMessage, a.message);
}

TEST(LtTaskEventMapper, ErrorAlertMapsToErrorEvent) {
  pfd::lt::LtAlertView a;
  a.type = pfd::lt::LtAlertView::Type::kTaskError;
  a.taskId = QUuid::createUuid();
  a.message = QStringLiteral("disk full");

  auto ev = pfd::lt::mapAlertToTaskEvent(a);
  ASSERT_TRUE(ev.has_value());
  EXPECT_EQ(ev->type, pfd::core::TaskEventType::kErrorOccurred);
  EXPECT_EQ(ev->errorMessage, QStringLiteral("disk full"));
}

TEST(LtTaskEventMapper, IgnoredOrNullTaskReturnsNullopt) {
  pfd::lt::LtAlertView ignored;
  ignored.type = pfd::lt::LtAlertView::Type::kIgnored;
  ignored.taskId = QUuid::createUuid();
  EXPECT_FALSE(pfd::lt::mapAlertToTaskEvent(ignored).has_value());

  pfd::lt::LtAlertView nullTask;
  nullTask.type = pfd::lt::LtAlertView::Type::kTaskAdded;
  // taskId 保持 null
  EXPECT_FALSE(pfd::lt::mapAlertToTaskEvent(nullTask).has_value());
}

}  // namespace
