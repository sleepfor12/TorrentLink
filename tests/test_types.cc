#include <QtCore/QUuid>

#include <gtest/gtest.h>

#include "base/types.h"

namespace {

using pfd::base::isError;
using pfd::base::isFinished;
using pfd::base::LogLevel;
using pfd::base::TaskId;
using pfd::base::TaskStatus;

TEST(Types, TaskIdIsQUuid) {
  const TaskId id = QUuid::createUuid();
  EXPECT_FALSE(id.isNull());
}

TEST(Types, TaskStatusIsFinished) {
  EXPECT_TRUE(isFinished(TaskStatus::kFinished));
  EXPECT_TRUE(isFinished(TaskStatus::kSeeding));
  EXPECT_FALSE(isFinished(TaskStatus::kDownloading));
  EXPECT_FALSE(isFinished(TaskStatus::kError));
}

TEST(Types, TaskStatusIsError) {
  EXPECT_TRUE(isError(TaskStatus::kError));
  EXPECT_FALSE(isError(TaskStatus::kDownloading));
}

TEST(Types, LogLevelValues) {
  EXPECT_LT(static_cast<int>(LogLevel::kDebug), static_cast<int>(LogLevel::kInfo));
  EXPECT_LT(static_cast<int>(LogLevel::kInfo), static_cast<int>(LogLevel::kWarning));
  EXPECT_LT(static_cast<int>(LogLevel::kWarning), static_cast<int>(LogLevel::kError));
  EXPECT_LT(static_cast<int>(LogLevel::kError), static_cast<int>(LogLevel::kFatal));
}

TEST(Types, LogLevelToString) {
  EXPECT_EQ(pfd::base::logLevelToString(LogLevel::kDebug), QStringLiteral("debug"));
  EXPECT_EQ(pfd::base::logLevelToString(LogLevel::kInfo), QStringLiteral("info"));
  EXPECT_EQ(pfd::base::logLevelToString(LogLevel::kWarning), QStringLiteral("warning"));
  EXPECT_EQ(pfd::base::logLevelToString(LogLevel::kError), QStringLiteral("error"));
  EXPECT_EQ(pfd::base::logLevelToString(LogLevel::kFatal), QStringLiteral("fatal"));
}

TEST(Types, TryParseLogLevelCaseInsensitive) {
  LogLevel level = LogLevel::kInfo;
  EXPECT_TRUE(pfd::base::tryParseLogLevel(QStringLiteral("DEBUG"), &level));
  EXPECT_EQ(level, LogLevel::kDebug);
  EXPECT_TRUE(pfd::base::tryParseLogLevel(QStringLiteral("warn"), &level));
  EXPECT_EQ(level, LogLevel::kWarning);
  EXPECT_TRUE(pfd::base::tryParseLogLevel(QStringLiteral(" Fatal "), &level));
  EXPECT_EQ(level, LogLevel::kFatal);
}

TEST(Types, TryParseLogLevelRejectsInvalidInput) {
  LogLevel level = LogLevel::kInfo;
  EXPECT_FALSE(pfd::base::tryParseLogLevel(QStringLiteral("verbose"), &level));
  EXPECT_FALSE(pfd::base::tryParseLogLevel(QStringLiteral(""), &level));
  EXPECT_FALSE(pfd::base::tryParseLogLevel(QStringLiteral("error"), nullptr));
}

}  // namespace
