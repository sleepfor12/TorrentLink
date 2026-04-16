#include <gtest/gtest.h>

#include "base/process_utils.h"

namespace {

using pfd::base::buildDetachedCommand;
using pfd::base::withPlatformExecutableSuffix;

TEST(ProcessUtils, BuildDetachedCommandParsesProgramAndArgs) {
  const auto cmd =
      buildDetachedCommand(QStringLiteral("mpv --no-video"), {QStringLiteral("a.mp4")});
  EXPECT_TRUE(cmd.isValid());
  EXPECT_FALSE(cmd.program.isEmpty());
  EXPECT_EQ(cmd.arguments.size(), 2);
  EXPECT_EQ(cmd.arguments[0], QStringLiteral("--no-video"));
  EXPECT_EQ(cmd.arguments[1], QStringLiteral("a.mp4"));
}

TEST(ProcessUtils, BuildDetachedCommandReturnsInvalidWhenEmpty) {
  const auto cmd = buildDetachedCommand(QStringLiteral("   "));
  EXPECT_FALSE(cmd.isValid());
}

TEST(ProcessUtils, ExecutableSuffixMatchesPlatformConvention) {
#ifdef _WIN32
  EXPECT_EQ(withPlatformExecutableSuffix(QStringLiteral("vlc")), QStringLiteral("vlc.exe"));
  EXPECT_EQ(withPlatformExecutableSuffix(QStringLiteral("python.exe")),
            QStringLiteral("python.exe"));
#else
  EXPECT_EQ(withPlatformExecutableSuffix(QStringLiteral("vlc")), QStringLiteral("vlc"));
#endif
}

}  // namespace
