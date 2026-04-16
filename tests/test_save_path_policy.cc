#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>

#include <gtest/gtest.h>

#include "core/save_path_policy.h"

using pfd::core::ContentLayout;
using pfd::core::SavePathPolicy;

class SavePathPolicyTest : public ::testing::Test {
protected:
  void SetUp() override {
    ASSERT_TRUE(tmpDir_.isValid());
  }
  QTemporaryDir tmpDir_;
};

TEST_F(SavePathPolicyTest, ResolveUsesInputIfGiven) {
  SavePathPolicy p;
  const QString input = tmpDir_.filePath("custom");
  const auto result = p.resolve(input);
  EXPECT_EQ(result, input);
  EXPECT_TRUE(QDir(result).exists());
}

TEST_F(SavePathPolicyTest, ResolveUsesDefaultWhenInputEmpty) {
  SavePathPolicy p;
  const QString defaultDir = tmpDir_.filePath("default_dl");
  p.setDefaultDownloadDir(defaultDir);
  const auto result = p.resolve(QString());
  EXPECT_EQ(result, defaultDir);
  EXPECT_TRUE(QDir(result).exists());
}

TEST_F(SavePathPolicyTest, ResolveUsesFallbackWhenBothEmpty) {
  SavePathPolicy p;
  const auto result = p.resolve(QString());
  const QString expected = QDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))
                               .filePath(QStringLiteral("p2p"));
  EXPECT_EQ(result, expected);
}

TEST_F(SavePathPolicyTest, ResolveTrimsWhitespace) {
  SavePathPolicy p;
  const QString input = "  " + tmpDir_.filePath("trimmed") + "  ";
  const auto result = p.resolve(input);
  EXPECT_EQ(result, tmpDir_.filePath("trimmed"));
}

TEST_F(SavePathPolicyTest, ResolveNormalizesMixedSeparators) {
  SavePathPolicy p;
  const QString raw = tmpDir_.path() + QStringLiteral("/a\\b/./c");
  const auto result = p.resolve(raw);
  EXPECT_EQ(result, QDir::cleanPath(raw));
}

TEST_F(SavePathPolicyTest, SetDefaultDirTrimsWhitespace) {
  SavePathPolicy p;
  const QString dir = tmpDir_.filePath("ws_test");
  p.setDefaultDownloadDir("  " + dir + "  ");
  const auto result = p.resolve(QString());
  EXPECT_EQ(result, dir);
}

TEST_F(SavePathPolicyTest, ApplyLayoutOriginal) {
  SavePathPolicy p;
  const QString basePath = tmpDir_.filePath("base");
  const auto result = p.applyLayout(basePath, "MyTorrent", ContentLayout::kOriginal);
  EXPECT_EQ(result, basePath);
}

TEST_F(SavePathPolicyTest, ApplyLayoutCreateSubfolder) {
  SavePathPolicy p;
  const QString basePath = tmpDir_.filePath("base");
  const auto result = p.applyLayout(basePath, "MyTorrent", ContentLayout::kCreateSubfolder);
  EXPECT_EQ(result, QDir(basePath).filePath("MyTorrent"));
}

TEST_F(SavePathPolicyTest, ApplyLayoutNoSubfolder) {
  SavePathPolicy p;
  const QString basePath = tmpDir_.filePath("base");
  const auto result = p.applyLayout(basePath, "MyTorrent", ContentLayout::kNoSubfolder);
  EXPECT_EQ(result, basePath);
}

TEST_F(SavePathPolicyTest, EnsureExistsCreatesDir) {
  const QString path = tmpDir_.filePath("a/b/c");
  ASSERT_FALSE(QDir(path).exists());
  SavePathPolicy::ensureExists(path);
  EXPECT_TRUE(QDir(path).exists());
}
