#include <gtest/gtest.h>

#include "base/text_encoding.h"

namespace {

TEST(TextEncoding, IsValidUtf8AcceptsAscii) {
  const char data[] = "Connection refused";
  EXPECT_TRUE(pfd::base::isValidUtf8(data, static_cast<int>(sizeof(data) - 1)));
}

TEST(TextEncoding, IsValidUtf8AcceptsChinese) {
  const std::string data = u8"连接被拒绝";
  EXPECT_TRUE(pfd::base::isValidUtf8(data.data(), static_cast<int>(data.size())));
}

TEST(TextEncoding, IsValidUtf8RejectsTruncatedSequence) {
  const char data[] = "\xE4\xB8";
  EXPECT_FALSE(pfd::base::isValidUtf8(data, 2));
}

TEST(TextEncoding, QStringFromStdBytesDecodesUtf8) {
  const std::string data = u8"invalid request";
  EXPECT_EQ(pfd::base::QStringFromStdBytes(data), QString::fromUtf8(u8"invalid request"));
}

TEST(TextEncoding, QStringFromStdBytesDecodesUtf8Chinese) {
  const std::string data = u8"未注册";
  EXPECT_EQ(pfd::base::QStringFromStdBytes(data), QString::fromUtf8(u8"未注册"));
}

TEST(TextEncoding, SanitizeHumanReadableTextReplacesControlChars) {
  const QString raw = QStringLiteral("bad\u0001message");
  const QString out = pfd::base::sanitizeHumanReadableText(raw);
  EXPECT_FALSE(out.contains(QChar(0x0001)));
  EXPECT_TRUE(out.contains(QChar::ReplacementCharacter));
}

}  // namespace
