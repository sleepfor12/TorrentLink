#include <QtNetwork/QHostAddress>

#include <gtest/gtest.h>

#include "core/network_util.h"

TEST(NetworkUtilTest, NormalizeBindModeDefaultsToLocalhost) {
  EXPECT_EQ(pfd::core::normalizeBuiltinTrackerBindMode(QStringLiteral("")),
            QStringLiteral("localhost"));
  EXPECT_EQ(pfd::core::normalizeBuiltinTrackerBindMode(QStringLiteral("unknown")),
            QStringLiteral("localhost"));
  EXPECT_EQ(pfd::core::normalizeBuiltinTrackerBindMode(QStringLiteral("LAN")),
            QStringLiteral("lan"));
}

TEST(NetworkUtilTest, ResolveBindAddress) {
  EXPECT_EQ(pfd::core::resolveBuiltinTrackerBindAddress(QStringLiteral("localhost")),
            QHostAddress(QHostAddress::LocalHost));
  EXPECT_EQ(pfd::core::resolveBuiltinTrackerBindAddress(QStringLiteral("lan")),
            QHostAddress(QHostAddress::AnyIPv4));
}

TEST(NetworkUtilTest, PrimaryPrivateIPv4WhenPresent) {
  const QString ip = pfd::core::primaryPrivateIPv4Address();
  if (ip.isEmpty()) {
    GTEST_SKIP() << "no usable LAN IPv4 in test environment";
  }
  const QHostAddress addr(ip);
  ASSERT_EQ(addr.protocol(), QAbstractSocket::IPv4Protocol);
  EXPECT_FALSE(addr.isLoopback());
}
