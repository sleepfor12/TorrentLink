#include <QtCore/QDir>
#include <QtCore/QFile>

#include <gtest/gtest.h>

#include "base/fs_utils.h"

namespace {

TEST(FsUtils, FileAndDirExists) {
  const QString base = QDir::tempPath() + QStringLiteral("/p2p_fs_utils_test");
  QDir().mkpath(base);
  const QString file = base + QStringLiteral("/a.txt");
  {
    QFile f(file);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("abc");
  }

  EXPECT_TRUE(pfd::base::dirExists(base));
  EXPECT_TRUE(pfd::base::fileExists(file));
  EXPECT_FALSE(pfd::base::fileExists(base));
  EXPECT_FALSE(pfd::base::dirExists(file));

  QDir(base).removeRecursively();
}

TEST(FsUtils, FileSizeAndNotFound) {
  const QString base = QDir::tempPath() + QStringLiteral("/p2p_fs_utils_size_test");
  QDir().mkpath(base);
  const QString file = base + QStringLiteral("/b.txt");
  {
    QFile f(file);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("hello");
  }

  EXPECT_EQ(pfd::base::fileSize(file), 5);
  EXPECT_EQ(pfd::base::fileSize(base + QStringLiteral("/nope.txt")), -1);

  QDir(base).removeRecursively();
}

TEST(FsUtils, AvailableBytes) {
  const qint64 bytes1 = pfd::base::availableBytes(QString());
  EXPECT_GT(bytes1, 0);
  const qint64 bytes2 = pfd::base::availableBytes(QDir::homePath());
  EXPECT_GT(bytes2, 0);
}

}  // namespace
