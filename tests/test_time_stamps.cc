#include <QtCore/QDateTime>

#include <gtest/gtest.h>

#include "base/time_stamps.h"

namespace {

using pfd::base::currentMs;
using pfd::base::fromDateTime;
using pfd::base::invalidMs;
using pfd::base::isValidMs;
using pfd::base::toDateTime;

TEST(TimeStamps, InvalidMsIsMinusOne) {
  EXPECT_EQ(invalidMs(), -1);
}

TEST(TimeStamps, IsValidMsRejectsInvalid) {
  EXPECT_FALSE(isValidMs(invalidMs()));
  EXPECT_FALSE(isValidMs(-2));
  EXPECT_FALSE(isValidMs(-100));
}

TEST(TimeStamps, IsValidMsRejectsNegative) {
  EXPECT_FALSE(isValidMs(-1));
}

TEST(TimeStamps, IsValidMsAcceptsPositive) {
  EXPECT_TRUE(isValidMs(0));
  EXPECT_TRUE(isValidMs(1));
  EXPECT_TRUE(isValidMs(1700000000000));  // 2023 年左右
}

TEST(TimeStamps, CurrentMsIsReasonable) {
  const qint64 now = currentMs();
  EXPECT_TRUE(isValidMs(now));
  // 应在合理范围：2020–2030 年（约 1577836800000–1893456000000）
  EXPECT_GE(now, 1577836800000LL);
  EXPECT_LE(now, 2000000000000LL);
}

TEST(TimeStamps, FromDateTimeToDateTimeRoundTrip) {
  const QDateTime dt(QDate(2024, 3, 19), QTime(12, 30, 45, 123), Qt::UTC);
  const qint64 ms = fromDateTime(dt);
  EXPECT_TRUE(isValidMs(ms));
  const QDateTime back = toDateTime(ms);
  EXPECT_TRUE(back.isValid());
  EXPECT_EQ(back, dt);
}

TEST(TimeStamps, FromDateTimeInvalidReturnsInvalidMs) {
  const QDateTime invalid;
  EXPECT_FALSE(invalid.isValid());
  EXPECT_EQ(fromDateTime(invalid), invalidMs());
}

TEST(TimeStamps, ToDateTimeInvalidMsReturnsInvalidQDateTime) {
  const QDateTime result = toDateTime(invalidMs());
  EXPECT_FALSE(result.isValid());
}

TEST(TimeStamps, ToDateTimeNegativeReturnsInvalidQDateTime) {
  EXPECT_FALSE(toDateTime(-100).isValid());
}

TEST(TimeStamps, ToDateTimeUsesUtc) {
  const QDateTime utc(QDate(2024, 3, 19), QTime(12, 30, 45, 123), Qt::UTC);
  const qint64 ms = fromDateTime(utc);
  const QDateTime dt = toDateTime(ms);
  EXPECT_TRUE(dt.isValid());
  EXPECT_EQ(dt.timeSpec(), Qt::UTC);
  EXPECT_EQ(dt, utc);
}

}  // namespace
