#include <QtCore/QDir>
#include <QtCore/QFile>

#include <gtest/gtest.h>

#include "base/io.h"

namespace {

using pfd::base::readWholeFile;
using pfd::base::readWholeFileAsText;
using pfd::base::writeWholeFile;

TEST(Io, WriteAndReadWholeFile) {
  const QString path = QDir::tempPath() + QStringLiteral("/p2p_io_test_write_read.dat");
  const QByteArray data("hello world\n");
  auto err = writeWholeFile(path, data);
  EXPECT_TRUE(err.isOk()) << err.displayMessage().toStdString();

  auto [read, readErr] = readWholeFile(path);
  EXPECT_TRUE(readErr.isOk());
  EXPECT_EQ(read, data);

  QFile::remove(path);
}

TEST(Io, WriteAndReadWholeFileAsText) {
  const QString path = QDir::tempPath() + QStringLiteral("/p2p_io_test_text.txt");
  const QString text(QStringLiteral("测试中文\nline2"));
  auto err = writeWholeFile(path, text);
  EXPECT_TRUE(err.isOk());

  auto [read, readErr] = readWholeFileAsText(path);
  EXPECT_TRUE(readErr.isOk());
  EXPECT_EQ(read, text);

  QFile::remove(path);
}

TEST(Io, ReadNonExistentReturnsError) {
  const QString path = QDir::tempPath() + QStringLiteral("/p2p_io_nonexistent_xyz");
  auto [data, err] = readWholeFile(path);
  EXPECT_TRUE(err.hasError());
  EXPECT_EQ(err.code(), pfd::base::ErrorCode::kFileNotFound);
  EXPECT_EQ(err.domain(), pfd::base::ErrorDomain::kIo);
  EXPECT_TRUE(err.detail().contains(QStringLiteral("path=")));
  EXPECT_TRUE(err.detail().contains(QStringLiteral("qt_file_error=")));
  EXPECT_TRUE(data.isEmpty());
}

TEST(Io, AtomicWriteOverwritesExisting) {
  const QString path = QDir::tempPath() + QStringLiteral("/p2p_io_atomic_overwrite.dat");
  EXPECT_TRUE(writeWholeFile(path, QByteArray("first")).isOk());
  auto err = writeWholeFile(path, QByteArray("second"));
  EXPECT_TRUE(err.isOk());

  auto [read, readErr] = readWholeFile(path);
  EXPECT_TRUE(readErr.isOk());
  EXPECT_EQ(read, QByteArray("second"));

  QFile::remove(path);
}

}  // namespace
