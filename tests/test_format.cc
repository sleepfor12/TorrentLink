#include <QtCore/QString>

#include <cmath>
#include <gtest/gtest.h>
#include <limits>

#include "base/format.h"

namespace {

using pfd::base::formatBytes;
using pfd::base::formatBytesIec;
using pfd::base::formatDuration;
using pfd::base::formatPercent;
using pfd::base::formatRate;
using pfd::base::formatRateBits;
using pfd::base::formatRatio;

TEST(Format, FormatBytesZero) {
  EXPECT_EQ(formatBytes(0), QStringLiteral("0 B"));
}

TEST(Format, FormatBytesSmall) {
  EXPECT_EQ(formatBytes(500), QStringLiteral("500 B"));
  EXPECT_EQ(formatBytes(1023), QStringLiteral("1023 B"));
}

TEST(Format, FormatBytesKilo) {
  EXPECT_EQ(formatBytes(1024), QStringLiteral("1 KB"));
  EXPECT_EQ(formatBytes(1536), QStringLiteral("1.5 KB"));
  EXPECT_EQ(formatBytes(1024 * 1024 - 1), QStringLiteral("1023.9 KB"));
}

TEST(Format, FormatBytesMega) {
  EXPECT_EQ(formatBytes(1024 * 1024), QStringLiteral("1 MB"));
  EXPECT_EQ(formatBytes(1024 * 1024 * 2 + 512 * 1024), QStringLiteral("2.5 MB"));
}

TEST(Format, FormatBytesGiga) {
  EXPECT_EQ(formatBytes(1024LL * 1024 * 1024), QStringLiteral("1 GB"));
  EXPECT_EQ(formatBytes(1024LL * 1024 * 1024 * 2), QStringLiteral("2 GB"));
}

TEST(Format, FormatBytesNegativeTreatsAsZero) {
  EXPECT_EQ(formatBytes(-100), QStringLiteral("0 B"));
}

TEST(Format, FormatRate) {
  EXPECT_EQ(formatRate(0), QStringLiteral("0 B/s"));
  EXPECT_EQ(formatRate(1024), QStringLiteral("1 KB/s"));
  EXPECT_EQ(formatRate(1024 * 1024 * 2), QStringLiteral("2 MB/s"));
}

TEST(Format, FormatDurationZero) {
  EXPECT_EQ(formatDuration(0), QStringLiteral("0s"));
}

TEST(Format, FormatDurationSeconds) {
  EXPECT_EQ(formatDuration(30), QStringLiteral("30s"));
  EXPECT_EQ(formatDuration(59), QStringLiteral("59s"));
}

TEST(Format, FormatDurationMinutes) {
  EXPECT_EQ(formatDuration(60), QStringLiteral("1m"));
  EXPECT_EQ(formatDuration(90), QStringLiteral("1m 30s"));
  EXPECT_EQ(formatDuration(150), QStringLiteral("2m 30s"));
}

TEST(Format, FormatDurationHours) {
  EXPECT_EQ(formatDuration(3600), QStringLiteral("1h"));
  EXPECT_EQ(formatDuration(3661), QStringLiteral("1h 1m 1s"));
}

TEST(Format, FormatDurationDays) {
  EXPECT_EQ(formatDuration(86400), QStringLiteral("1d"));
  EXPECT_EQ(formatDuration(90061), QStringLiteral("1d 1h 1m 1s"));
}

TEST(Format, FormatDurationNegativeTreatsAsZero) {
  EXPECT_EQ(formatDuration(-100), QStringLiteral("0s"));
}

TEST(Format, FormatBytesIec) {
  EXPECT_EQ(formatBytesIec(0), QStringLiteral("0 B"));
  EXPECT_EQ(formatBytesIec(1024), QStringLiteral("1 KiB"));
  EXPECT_EQ(formatBytesIec(1024 * 1024), QStringLiteral("1 MiB"));
  EXPECT_EQ(formatBytesIec(1024LL * 1024 * 1024), QStringLiteral("1 GiB"));
  EXPECT_EQ(formatBytesIec(1536), QStringLiteral("1.5 KiB"));
}

TEST(Format, FormatRateBits) {
  EXPECT_EQ(formatRateBits(0), QStringLiteral("0 bps"));
  EXPECT_EQ(formatRateBits(1000), QStringLiteral("1 Kbps"));
  EXPECT_EQ(formatRateBits(1500000), QStringLiteral("1.5 Mbps"));
}

TEST(Format, FormatPercent) {
  EXPECT_EQ(formatPercent(0.0), QStringLiteral("0.0%"));
  EXPECT_EQ(formatPercent(1.0), QStringLiteral("100.0%"));
  EXPECT_EQ(formatPercent(0.5), QStringLiteral("50.0%"));
  EXPECT_EQ(formatPercent(0.753), QStringLiteral("75.3%"));
  EXPECT_EQ(formatPercent(-0.1), QStringLiteral("0.0%"));
  EXPECT_EQ(formatPercent(1.5), QStringLiteral("100.0%"));
}

TEST(Format, FormatRatio) {
  EXPECT_EQ(formatRatio(0.0), QStringLiteral("0.0"));
  EXPECT_EQ(formatRatio(1.5), QStringLiteral("1.5"));
  EXPECT_EQ(formatRatio(10.25), QStringLiteral("10.3"));  // 四舍五入
  EXPECT_EQ(formatRatio(-1.0), QStringLiteral("∞"));
  EXPECT_EQ(formatRatio(std::numeric_limits<double>::infinity()), QStringLiteral("∞"));
  EXPECT_EQ(formatRatio(-std::numeric_limits<double>::infinity()), QStringLiteral("∞"));
  EXPECT_TRUE(std::isnan(NAN));
  EXPECT_EQ(formatRatio(NAN), QStringLiteral("∞"));
}

}  // namespace
