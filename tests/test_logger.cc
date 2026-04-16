#include <QtCore/QDate>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>

#include <gtest/gtest.h>
#include <thread>
#include <vector>

#include "core/logger.h"

namespace {

using pfd::base::LogLevel;
using pfd::core::logger::Logger;

QString expectedDatedPath(const QString& raw_path) {
  const QFileInfo fi(raw_path);
  const QString date = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
  return fi.absolutePath() + QStringLiteral("/") + fi.completeBaseName() + QStringLiteral("-") +
         date + QStringLiteral(".") + fi.suffix();
}

TEST(Logger, WritesToFileWhenPathSet) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("app.log"));

  Logger& log = Logger::instance();
  log.setLogLevel(LogLevel::kDebug);
  log.clearGlobalLogs();
  log.setLogFilePath(path);

  LOG_INFO(QStringLiteral("hello_logger"));
  log.flush();

  const QString datedPath = expectedDatedPath(path);
  QFile f(datedPath);
  ASSERT_TRUE(f.open(QIODevice::ReadOnly | QIODevice::Text));
  const QString content = QString::fromUtf8(f.readAll());
  EXPECT_TRUE(content.contains(QStringLiteral("hello_logger")));
  EXPECT_TRUE(content.contains(QStringLiteral("[info]")));

  log.setLogFilePath(QStringLiteral(""));
}

TEST(Logger, GlobalSnapshotReflectsRingBuffer) {
  Logger& log = Logger::instance();
  log.setLogLevel(LogLevel::kDebug);
  log.clearGlobalLogs();
  log.setLogFilePath(QStringLiteral(""));

  LOG_WARN(QStringLiteral("snap_one"));
  const auto snap = log.globalSnapshot(10);
  ASSERT_EQ(snap.size(), 1u);
  EXPECT_EQ(snap[0].message, QStringLiteral("snap_one"));
  EXPECT_EQ(snap[0].level, LogLevel::kWarning);

  log.clearGlobalLogs();
  EXPECT_TRUE(log.globalSnapshot(10).empty());
  EXPECT_EQ(log.droppedAsyncCmdCount(), 0u);
}

TEST(Logger, RotationOverwritesOldestArchive) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("rotate.log"));

  Logger& log = Logger::instance();
  log.setLogLevel(LogLevel::kDebug);
  log.clearGlobalLogs();
  log.setLogFilePath(path);
  const QString datedPath = expectedDatedPath(path);
  log.setRotationPolicy(/*max_file_size_bytes=*/120, /*max_backup_files=*/2);

  for (int i = 0; i < 20; ++i) {
    LOG_INFO(QStringLiteral("rotation_line_%1_xxxxxxxxxx").arg(i));
  }
  log.flush();

  EXPECT_TRUE(QFile::exists(datedPath));
  EXPECT_TRUE(QFile::exists(datedPath + QStringLiteral(".1")));
  EXPECT_TRUE(QFile::exists(datedPath + QStringLiteral(".2")));
  EXPECT_FALSE(QFile::exists(datedPath + QStringLiteral(".3")));

  QFile oldest(datedPath + QStringLiteral(".2"));
  ASSERT_TRUE(oldest.open(QIODevice::ReadOnly | QIODevice::Text));
  const QString oldestContent = QString::fromUtf8(oldest.readAll());
  // 说明最旧归档已被覆盖，不会长期保留最早的行。
  EXPECT_FALSE(oldestContent.contains(QStringLiteral("rotation_line_0")));

  log.setRotationPolicy(/*max_file_size_bytes=*/0, /*max_backup_files=*/3);
  log.setLogFilePath(QStringLiteral(""));
}

TEST(Logger, StressConcurrentLogging) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = dir.filePath(QStringLiteral("stress.log"));

  Logger& log = Logger::instance();
  log.setLogLevel(LogLevel::kInfo);
  log.clearGlobalLogs();
  log.setLogFilePath(path);
  log.setRotationPolicy(/*max_file_size_bytes=*/0, /*max_backup_files=*/3);

  constexpr int kThreads = 8;
  constexpr int kPerThread = 20000;
  std::vector<std::thread> workers;
  workers.reserve(kThreads);

  for (int t = 0; t < kThreads; ++t) {
    workers.emplace_back([t]() {
      for (int i = 0; i < kPerThread; ++i) {
        LOG_INFO(QStringLiteral("stress t=%1 i=%2").arg(t).arg(i));
      }
    });
  }
  for (auto& w : workers) {
    w.join();
  }

  log.flush();
  const auto snap = log.globalSnapshot(128);
  EXPECT_FALSE(snap.empty());

  log.setLogFilePath(QStringLiteral(""));
}

}  // namespace
