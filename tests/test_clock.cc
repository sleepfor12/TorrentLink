#include <gtest/gtest.h>

#include "base/clock.h"

namespace {

TEST(Clock, DefaultNowMsReasonable) {
  pfd::base::Clock c;
  EXPECT_GT(c.nowMs(), 0);
}

TEST(Clock, CustomProviderOverridesNow) {
  pfd::base::Clock c;
  c.setNowMsProvider([]() { return 123456789LL; });
  EXPECT_EQ(c.nowMs(), 123456789LL);
}

TEST(Clock, ResetRestoresDefaultProvider) {
  pfd::base::Clock c;
  c.setNowMsProvider([]() { return 42LL; });
  EXPECT_EQ(c.nowMs(), 42LL);
  c.reset();
  EXPECT_GT(c.nowMs(), 0);
  EXPECT_NE(c.nowMs(), 42LL);
}

}  // namespace
