#include <QtCore/QStringList>
#include <QtCore/QUuid>

#include <gtest/gtest.h>

#include "app/task_control_command_use_case.h"

namespace {

TEST(TaskControlCommandUseCase, DispatchesAllCommandsAndLogs) {
  int pauseCount = 0;
  int resumeCount = 0;
  int removeCount = 0;
  int connectionsCount = 0;
  int forceStartCount = 0;
  int forceRecheckCount = 0;
  int forceReannounceCount = 0;
  int sequentialCount = 0;
  int autoManagedCount = 0;
  int logCount = 0;

  pfd::base::TaskId lastTaskId;
  bool lastRemoveFiles = false;
  int lastLimit = -1;
  bool lastBool = false;
  QString lastLog;

  pfd::app::TaskControlCommandUseCase useCase(
      [&](const pfd::base::TaskId& id) {
        ++pauseCount;
        lastTaskId = id;
      },
      [&](const pfd::base::TaskId& id) {
        ++resumeCount;
        lastTaskId = id;
      },
      [&](const pfd::base::TaskId& id, bool removeFiles) {
        ++removeCount;
        lastTaskId = id;
        lastRemoveFiles = removeFiles;
      },
      [&](const pfd::base::TaskId& id, int limit) {
        ++connectionsCount;
        lastTaskId = id;
        lastLimit = limit;
      },
      [&](const pfd::base::TaskId& id) {
        ++forceStartCount;
        lastTaskId = id;
      },
      [&](const pfd::base::TaskId& id) {
        ++forceRecheckCount;
        lastTaskId = id;
      },
      [&](const pfd::base::TaskId& id) {
        ++forceReannounceCount;
        lastTaskId = id;
      },
      [&](const pfd::base::TaskId& id, bool enabled) {
        ++sequentialCount;
        lastTaskId = id;
        lastBool = enabled;
      },
      [&](const pfd::base::TaskId& id, bool enabled) {
        ++autoManagedCount;
        lastTaskId = id;
        lastBool = enabled;
      },
      [&](const QString& msg) {
        ++logCount;
        lastLog = msg;
      });

  const auto taskId = QUuid::createUuid();
  useCase.pauseTask(taskId);
  useCase.resumeTask(taskId);
  useCase.removeTask(taskId, true);
  useCase.setTaskConnectionsLimit(taskId, 42);
  useCase.forceStartTask(taskId);
  useCase.forceRecheckTask(taskId);
  useCase.forceReannounceTask(taskId);
  useCase.setSequentialDownload(taskId, true);
  useCase.setAutoManagedTask(taskId, false);

  EXPECT_EQ(pauseCount, 1);
  EXPECT_EQ(resumeCount, 1);
  EXPECT_EQ(removeCount, 1);
  EXPECT_EQ(connectionsCount, 1);
  EXPECT_EQ(forceStartCount, 1);
  EXPECT_EQ(forceRecheckCount, 1);
  EXPECT_EQ(forceReannounceCount, 1);
  EXPECT_EQ(sequentialCount, 1);
  EXPECT_EQ(autoManagedCount, 1);
  EXPECT_EQ(logCount, 9);
  EXPECT_EQ(lastTaskId, taskId);
  EXPECT_TRUE(lastRemoveFiles);
  EXPECT_EQ(lastLimit, 42);
  EXPECT_FALSE(lastBool);
  EXPECT_TRUE(lastLog.contains(QStringLiteral("Auto managed changed")));
}

}  // namespace
