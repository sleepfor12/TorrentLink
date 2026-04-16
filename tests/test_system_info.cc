#include <gtest/gtest.h>

#include "base/system_info.h"

namespace {

TEST(SystemInfo, ProcessIdPositive) {
  EXPECT_GT(pfd::base::processId(), 0);
}

TEST(SystemInfo, HostNameNotEmpty) {
  EXPECT_FALSE(pfd::base::hostName().isEmpty());
}

TEST(SystemInfo, UserNameMayBeEmptyButIsStableType) {
  const QString user = pfd::base::userName();
  EXPECT_TRUE(user.isEmpty() || !user.trimmed().isEmpty());
}

}  // namespace
