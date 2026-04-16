#include <gtest/gtest.h>

#include "base/input_sanitizer.h"

namespace {

using pfd::base::validatePath;
using pfd::base::validateTorrentFilePath;

TEST(InputSanitizer, RejectsUnixTraversalPath) {
  const auto err = validatePath(QStringLiteral("../downloads"));
  EXPECT_TRUE(err.hasError());
}

TEST(InputSanitizer, RejectsWindowsTraversalPath) {
  const auto err = validatePath(QStringLiteral("..\\downloads"));
  EXPECT_TRUE(err.hasError());
}

TEST(InputSanitizer, AcceptsWindowsStyleNormalPath) {
  const auto err = validatePath(QStringLiteral("C:\\Users\\Alice\\Downloads"));
  EXPECT_TRUE(err.isOk());
}

TEST(InputSanitizer, AcceptsWindowsUncPath) {
  const auto err = validatePath(QStringLiteral("\\\\server\\share\\downloads"));
  EXPECT_TRUE(err.isOk());
}

TEST(InputSanitizer, RejectsWindowsUncTraversalPath) {
  const auto err = validatePath(QStringLiteral("\\\\server\\share\\..\\secret"));
  EXPECT_TRUE(err.hasError());
}

TEST(InputSanitizer, RejectsMixedSeparatorTraversalPath) {
  const auto err = validatePath(QStringLiteral("C:/downloads\\..\\secret"));
  EXPECT_TRUE(err.hasError());
}

TEST(InputSanitizer, RejectsTorrentFileWithoutSuffix) {
  const auto err = validateTorrentFilePath(QStringLiteral("C:\\tmp\\abc.txt"));
  EXPECT_TRUE(err.hasError());
}

TEST(InputSanitizer, AcceptsTorrentFileWithSuffixAndWindowsSeparators) {
  const auto err = validateTorrentFilePath(QStringLiteral("C:\\tmp\\abc.torrent"));
  EXPECT_TRUE(err.isOk());
}

}  // namespace
