#include <QtCore/QUuid>

#include <gtest/gtest.h>

#include "core/task_state_store.h"

namespace {

using pfd::base::TaskStatus;
using pfd::core::TaskEvent;
using pfd::core::TaskEventType;
using pfd::core::TaskStateStore;

TEST(TaskStateStore, ApplyMetaAndProgressProducesSnapshot) {
  TaskStateStore store;
  const auto id = QUuid::createUuid();

  TaskEvent meta;
  meta.type = TaskEventType::kUpsertMeta;
  meta.taskId = id;
  meta.name = QStringLiteral("ubuntu.iso");
  meta.savePath = QStringLiteral("/tmp");
  meta.totalBytes = 1000;
  store.applyEvent(meta);

  TaskEvent progress;
  progress.type = TaskEventType::kProgressUpdated;
  progress.taskId = id;
  progress.downloadedBytes = 250;
  progress.uploadedBytes = 10;
  store.applyEvent(progress);

  auto snap = store.snapshot(id);
  ASSERT_TRUE(snap.has_value());
  EXPECT_EQ(snap->name, QStringLiteral("ubuntu.iso"));
  EXPECT_EQ(snap->savePath, QStringLiteral("/tmp"));
  EXPECT_EQ(snap->totalBytes, 1000);
  EXPECT_EQ(snap->downloadedBytes, 250);
  EXPECT_EQ(snap->uploadedBytes, 10);
  EXPECT_DOUBLE_EQ(snap->progress01(), 0.25);
}

TEST(TaskStateStore, ProgressEventCanUpdateStatus) {
  TaskStateStore store;
  const auto id = QUuid::createUuid();

  TaskEvent meta;
  meta.type = TaskEventType::kUpsertMeta;
  meta.taskId = id;
  meta.name = QStringLiteral("debian.iso");
  store.applyEvent(meta);

  TaskEvent progress;
  progress.type = TaskEventType::kProgressUpdated;
  progress.taskId = id;
  progress.status = TaskStatus::kSeeding;
  progress.downloadedBytes = 100;
  progress.totalBytes = 100;
  store.applyEvent(progress);

  auto snap = store.snapshot(id);
  ASSERT_TRUE(snap.has_value());
  EXPECT_EQ(snap->status, TaskStatus::kSeeding);
  EXPECT_EQ(snap->downloadedBytes, 100);
  EXPECT_EQ(snap->totalBytes, 100);
}

TEST(TaskStateStore, ErrorEventForcesErrorStatus) {
  TaskStateStore store;
  const auto id = QUuid::createUuid();

  TaskEvent status;
  status.type = TaskEventType::kStatusChanged;
  status.taskId = id;
  status.status = TaskStatus::kDownloading;
  store.applyEvent(status);

  TaskEvent err;
  err.type = TaskEventType::kErrorOccurred;
  err.taskId = id;
  err.errorMessage = QStringLiteral("disk full");
  store.applyEvent(err);

  auto snap = store.snapshot(id);
  ASSERT_TRUE(snap.has_value());
  EXPECT_EQ(snap->status, TaskStatus::kError);
  EXPECT_EQ(snap->errorMessage, QStringLiteral("disk full"));
}

TEST(TaskStateStore, DuplicateRejectedIsNoOp) {
  TaskStateStore store;
  const auto id = QUuid::createUuid();

  TaskEvent meta;
  meta.type = TaskEventType::kUpsertMeta;
  meta.taskId = id;
  meta.name = QStringLiteral("keep-name");
  store.applyEvent(meta);

  TaskEvent dup;
  dup.type = TaskEventType::kDuplicateRejected;
  dup.taskId = id;
  dup.errorMessage = QStringLiteral("would confuse if applied as error");
  store.applyEvent(dup);

  auto snap = store.snapshot(id);
  ASSERT_TRUE(snap.has_value());
  EXPECT_EQ(snap->name, QStringLiteral("keep-name"));
  EXPECT_NE(snap->status, TaskStatus::kError);
  EXPECT_TRUE(snap->errorMessage.isEmpty());
}

TEST(TaskStateStore, RemovedEventDeletesSnapshot) {
  TaskStateStore store;
  const auto id = QUuid::createUuid();

  TaskEvent meta;
  meta.type = TaskEventType::kUpsertMeta;
  meta.taskId = id;
  meta.name = QStringLiteral("x");
  store.applyEvent(meta);
  ASSERT_TRUE(store.snapshot(id).has_value());

  TaskEvent removed;
  removed.type = TaskEventType::kRemoved;
  removed.taskId = id;
  store.applyEvent(removed);

  EXPECT_FALSE(store.snapshot(id).has_value());
}

}  // namespace
