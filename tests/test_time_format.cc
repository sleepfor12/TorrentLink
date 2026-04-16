#include <QtCore/QDateTime>

#include <gtest/gtest.h>

#include "base/time_format.h"
#include "base/time_stamps.h"

namespace {

using pfd::base::formatDateTime;
using pfd::base::formatIso8601;
using pfd::base::fromDateTime;
using pfd::base::invalidMs;
using pfd::base::toDateTime;

TEST(TimeFormat, FormatIso8601ValidMs) {
  const QDateTime utc(QDate(2024, 3, 19), QTime(12, 30, 45, 123), Qt::UTC);
  const qint64 ms = fromDateTime(utc);
  const QString result = formatIso8601(ms);
  EXPECT_FALSE(result.isEmpty());
  EXPECT_TRUE(result.contains(QLatin1Char('.')));
  EXPECT_TRUE(result.contains(QStringLiteral("2024")));
  EXPECT_TRUE(result.contains(QStringLiteral("03")));
  EXPECT_TRUE(result.contains(QStringLiteral("19")));
  EXPECT_TRUE(result.contains(QStringLiteral("12")));
  EXPECT_TRUE(result.contains(QStringLiteral("30")));
  EXPECT_TRUE(result.contains(QStringLiteral("45")));
}

TEST(TimeFormat, FormatIso8601InvalidMsReturnsEmpty) {
  EXPECT_TRUE(formatIso8601(invalidMs()).isEmpty());
  EXPECT_TRUE(formatIso8601(-100).isEmpty());
}

TEST(TimeFormat, FormatIso8601FromQDateTime) {
  const QDateTime dt(QDate(2024, 1, 15), QTime(8, 0, 0), Qt::UTC);
  const QString result = formatIso8601(dt);
  EXPECT_FALSE(result.isEmpty());
  EXPECT_TRUE(result.contains(QStringLiteral("2024")));
}

TEST(TimeFormat, FormatDateTimeValidMs) {
  const QDateTime utc(QDate(2024, 3, 19), QTime(12, 30, 45), Qt::UTC);
  const qint64 ms = fromDateTime(utc);
  const QString result = formatDateTime(ms);
  EXPECT_FALSE(result.isEmpty());
  EXPECT_TRUE(result.contains(QStringLiteral("2024")));
  EXPECT_TRUE(result.contains(QStringLiteral("03")));
  EXPECT_TRUE(result.contains(QStringLiteral("19")));
}

TEST(TimeFormat, FormatDateTimeInvalidMsReturnsEmpty) {
  EXPECT_TRUE(formatDateTime(invalidMs()).isEmpty());
}

TEST(TimeFormat, FormatDateTimeRoundTrip) {
  const QDateTime utc(QDate(2024, 6, 1), QTime(0, 0, 0), Qt::UTC);
  const qint64 ms = fromDateTime(utc);
  const QString formatted = formatDateTime(ms);
  EXPECT_FALSE(formatted.isEmpty());
  const QDateTime back = toDateTime(ms);
  EXPECT_EQ(fromDateTime(back), ms);
}

}  // namespace
