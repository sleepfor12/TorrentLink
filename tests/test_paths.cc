#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>

#include <gtest/gtest.h>

#include "base/paths.h"

namespace {

using pfd::base::appDataDir;
using pfd::base::configDir;
using pfd::base::ensureExists;
using pfd::base::resumeDir;

TEST(Paths, AppDataDirNonEmptyAndEndsWithSlash) {
  const QString dir = appDataDir();
  EXPECT_FALSE(dir.isEmpty());
  const QChar sep = QDir::separator();
  EXPECT_TRUE(dir.endsWith(sep) || dir.endsWith(QLatin1Char('/')) ||
              dir.endsWith(QLatin1Char('\\')));
}

TEST(Paths, AppDataDirContainsAppName) {
  const QString dir = appDataDir();
  EXPECT_TRUE(dir.contains(QStringLiteral("p2p_downloader")));
}

TEST(Paths, ConfigDirNonEmptyAndEndsWithSlash) {
  const QString dir = configDir();
  EXPECT_FALSE(dir.isEmpty());
  const QChar sep = QDir::separator();
  EXPECT_TRUE(dir.endsWith(sep) || dir.endsWith(QLatin1Char('/')) ||
              dir.endsWith(QLatin1Char('\\')));
}

TEST(Paths, ConfigDirContainsAppName) {
  const QString dir = configDir();
  EXPECT_TRUE(dir.contains(QStringLiteral("p2p_downloader")));
}

TEST(Paths, ResumeDirIsSubdirOfAppData) {
  const QString appData = appDataDir();
  const QString resume = resumeDir();
  EXPECT_FALSE(resume.isEmpty());
  EXPECT_TRUE(resume.startsWith(appData));
  QString resumeTrimmed = resume;
  while (resumeTrimmed.endsWith(QLatin1Char('/')) || resumeTrimmed.endsWith(QLatin1Char('\\'))) {
    resumeTrimmed.chop(1);
  }
  EXPECT_EQ(QFileInfo(resumeTrimmed).fileName(), QStringLiteral("resume"));
}

TEST(Paths, EnsureExistsCreatesDirectory) {
  const QString base = QDir::tempPath() + QStringLiteral("/p2p_paths_test");
  const QString nested = base + QStringLiteral("/a/b/c");
  EXPECT_TRUE(ensureExists(nested));
  EXPECT_TRUE(QDir(nested).exists());
  QDir(base).removeRecursively();
}

TEST(Paths, EnsureExistsIdempotent) {
  const QString path = QDir::tempPath() + QStringLiteral("/p2p_paths_idempotent");
  EXPECT_TRUE(ensureExists(path));
  EXPECT_TRUE(ensureExists(path));
  QDir(path).removeRecursively();
}

TEST(Paths, EnsureExistsEmptyPathReturnsFalse) {
  EXPECT_FALSE(ensureExists(QString()));
}

}  // namespace
