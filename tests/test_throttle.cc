#include <gtest/gtest.h>

#include "base/throttle.h"

namespace {

using pfd::base::Throttler;

TEST(Throttle, FirstCallShouldFireReturnsTrue) {
  Throttler t(500);
  EXPECT_TRUE(t.shouldFire(1000));
  EXPECT_TRUE(t.shouldFire(1000));  // 未 markFired 时仍为 true
}

TEST(Throttle, WithinIntervalReturnsFalse) {
  Throttler t(500);
  t.markFired(1000);
  EXPECT_FALSE(t.shouldFire(1200));  // 200ms < 500ms
  EXPECT_FALSE(t.shouldFire(1499));
}

TEST(Throttle, AfterIntervalReturnsTrue) {
  Throttler t(500);
  t.markFired(1000);
  EXPECT_TRUE(t.shouldFire(1500));  // 正好 500ms
  EXPECT_TRUE(t.shouldFire(1600));  // 超过
}

TEST(Throttle, MarkFiredThenCheckAgain) {
  Throttler t(500);
  EXPECT_TRUE(t.shouldFire(1000));
  t.markFired(1000);
  EXPECT_FALSE(t.shouldFire(1200));
  t.markFired(1200);                 // 实际执行
  EXPECT_FALSE(t.shouldFire(1500));  // 距 1200 仅 300ms
  EXPECT_TRUE(t.shouldFire(1800));   // 距 1200 已 600ms
}

TEST(Throttle, ResetMakesNextShouldFireTrue) {
  Throttler t(500);
  t.markFired(1000);
  EXPECT_FALSE(t.shouldFire(1200));
  t.reset();
  EXPECT_TRUE(t.shouldFire(1200));
}

TEST(Throttle, ZeroIntervalDefaultsToOne) {
  Throttler t(0);
  t.markFired(1000);
  EXPECT_TRUE(t.shouldFire(1001));  // 间隔 1ms 即够
}

}  // namespace
