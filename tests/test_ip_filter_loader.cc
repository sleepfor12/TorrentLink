#include <gtest/gtest.h>

#include <QtCore/QTemporaryFile>

#include <libtorrent/address.hpp>
#include <libtorrent/ip_filter.hpp>

#include <boost/system/error_code.hpp>

#include "lt/ip_filter_loader.h"

namespace {

static libtorrent::address makeAddr(const char* s) {
  boost::system::error_code ec;
  const auto a = libtorrent::make_address(std::string(s), ec);
  EXPECT_FALSE(ec) << s;
  return a;
}

TEST(IpFilterLoader, EmptyOrComment) {
  libtorrent::ip_filter f;
  EXPECT_TRUE(pfd::lt::appendIpFilterLine(QStringLiteral(""), &f));
  EXPECT_TRUE(pfd::lt::appendIpFilterLine(QStringLiteral("  # comment"), &f));
}

TEST(IpFilterLoader, SingleIpv4) {
  libtorrent::ip_filter f;
  ASSERT_TRUE(pfd::lt::appendIpFilterLine(QStringLiteral("192.0.2.10"), &f));
  EXPECT_EQ(f.access(makeAddr("192.0.2.10")), libtorrent::ip_filter::blocked);
}

TEST(IpFilterLoader, CidrV4) {
  libtorrent::ip_filter f;
  ASSERT_TRUE(pfd::lt::appendIpFilterLine(QStringLiteral("192.0.2.0/24"), &f));
  EXPECT_EQ(f.access(makeAddr("192.0.2.1")), libtorrent::ip_filter::blocked);
  EXPECT_EQ(f.access(makeAddr("192.0.3.1")), 0u);
}

TEST(IpFilterLoader, RangeV4) {
  libtorrent::ip_filter f;
  ASSERT_TRUE(pfd::lt::appendIpFilterLine(QStringLiteral("192.0.2.1 - 192.0.2.3"), &f));
  EXPECT_EQ(f.access(makeAddr("192.0.2.2")), libtorrent::ip_filter::blocked);
  EXPECT_EQ(f.access(makeAddr("192.0.2.4")), 0u);
}

TEST(IpFilterLoader, SingleIpv6) {
  libtorrent::ip_filter f;
  ASSERT_TRUE(pfd::lt::appendIpFilterLine(QStringLiteral("2001:db8::1"), &f));
  EXPECT_EQ(f.access(makeAddr("2001:db8::1")), libtorrent::ip_filter::blocked);
  EXPECT_EQ(f.access(makeAddr("2001:db8::2")), 0u);
}

TEST(IpFilterLoader, CidrV6) {
  libtorrent::ip_filter f;
  ASSERT_TRUE(pfd::lt::appendIpFilterLine(QStringLiteral("2001:db8::/32"), &f));
  EXPECT_EQ(f.access(makeAddr("2001:db8::1")), libtorrent::ip_filter::blocked);
  EXPECT_EQ(f.access(makeAddr("2001:db9::1")), 0u);
}

TEST(IpFilterLoader, RangeV6) {
  libtorrent::ip_filter f;
  ASSERT_TRUE(
      pfd::lt::appendIpFilterLine(QStringLiteral("2001:db8::1 - 2001:db8::3"), &f));
  EXPECT_EQ(f.access(makeAddr("2001:db8::2")), libtorrent::ip_filter::blocked);
  EXPECT_EQ(f.access(makeAddr("2001:db8::4")), 0u);
}

TEST(IpFilterLoader, MixedRangeV4V6Rejected) {
  libtorrent::ip_filter f;
  EXPECT_FALSE(pfd::lt::appendIpFilterLine(QStringLiteral("192.0.2.1 - 2001:db8::1"), &f));
}

TEST(IpFilterLoader, RejectInvalidGarbage) {
  libtorrent::ip_filter f;
  EXPECT_FALSE(pfd::lt::appendIpFilterLine(QStringLiteral("not_an_ip"), &f));
}

TEST(IpFilterLoader, RejectBadCidrV4Prefix) {
  libtorrent::ip_filter f;
  EXPECT_FALSE(pfd::lt::appendIpFilterLine(QStringLiteral("192.0.2.0/33"), &f));
}

TEST(IpFilterLoader, RejectBadCidrV6Prefix) {
  libtorrent::ip_filter f;
  EXPECT_FALSE(pfd::lt::appendIpFilterLine(QStringLiteral("2001:db8::/200"), &f));
}

TEST(IpFilterLoader, RangeV4ReversedEndpoints) {
  libtorrent::ip_filter f;
  ASSERT_TRUE(pfd::lt::appendIpFilterLine(QStringLiteral("192.0.2.3 - 192.0.2.1"), &f));
  EXPECT_EQ(f.access(makeAddr("192.0.2.2")), libtorrent::ip_filter::blocked);
}

TEST(IpFilterLoader, RangeV6ReversedEndpoints) {
  libtorrent::ip_filter f;
  ASSERT_TRUE(
      pfd::lt::appendIpFilterLine(QStringLiteral("2001:db8::3 - 2001:db8::1"), &f));
  EXPECT_EQ(f.access(makeAddr("2001:db8::2")), libtorrent::ip_filter::blocked);
}

TEST(IpFilterLoader, CidrV6Slash128) {
  libtorrent::ip_filter f;
  ASSERT_TRUE(pfd::lt::appendIpFilterLine(QStringLiteral("::1/128"), &f));
  EXPECT_EQ(f.access(makeAddr("::1")), libtorrent::ip_filter::blocked);
  EXPECT_EQ(f.access(makeAddr("::2")), 0u);
}

TEST(IpFilterLoader, BracketIpv6Single) {
  libtorrent::ip_filter f;
  ASSERT_TRUE(pfd::lt::appendIpFilterLine(QStringLiteral("[2001:db8::cafe]"), &f));
  EXPECT_EQ(f.access(makeAddr("2001:db8::cafe")), libtorrent::ip_filter::blocked);
}

TEST(IpFilterLoader, CidrV4Slash32) {
  libtorrent::ip_filter f;
  ASSERT_TRUE(pfd::lt::appendIpFilterLine(QStringLiteral("192.0.2.9/32"), &f));
  EXPECT_EQ(f.access(makeAddr("192.0.2.9")), libtorrent::ip_filter::blocked);
  EXPECT_EQ(f.access(makeAddr("192.0.2.10")), 0u);
}

TEST(IpFilterLoader, LoadIpFilterFile_NullOut) {
  QString err;
  EXPECT_FALSE(pfd::lt::loadIpFilterFile(QStringLiteral("/dev/null"), nullptr, nullptr, &err));
  EXPECT_EQ(err, QStringLiteral("out==null"));
}

TEST(IpFilterLoader, LoadIpFilterFile_MissingFile) {
  libtorrent::ip_filter f;
  QString err;
  EXPECT_FALSE(
      pfd::lt::loadIpFilterFile(QStringLiteral("/nonexistent/path/ip_filter_rules.txt"), &f,
                                nullptr, &err));
  EXPECT_EQ(err, QStringLiteral("cannot open file"));
}

TEST(IpFilterLoader, LoadIpFilterFile_CountsSuccessfulLines) {
  QTemporaryFile tmp;
  ASSERT_TRUE(tmp.open());
  tmp.write(
      "# header\n"
      "\n"
      "192.0.2.5\n"
      "oops-not-ip\n"
      "192.0.2.6\n");
  tmp.flush();
  tmp.close();

  libtorrent::ip_filter f;
  int applied = -1;
  QString err(QStringLiteral("unset"));
  ASSERT_TRUE(pfd::lt::loadIpFilterFile(tmp.fileName(), &f, &applied, &err));
  EXPECT_TRUE(err.isEmpty());
  EXPECT_EQ(applied, 2);
  EXPECT_EQ(f.access(makeAddr("192.0.2.5")), libtorrent::ip_filter::blocked);
  EXPECT_EQ(f.access(makeAddr("192.0.2.6")), libtorrent::ip_filter::blocked);
}

TEST(IpFilterLoader, LoadIpFilterFile_MultilineIpv6AndV4) {
  QTemporaryFile tmp;
  ASSERT_TRUE(tmp.open());
  tmp.write(
      "2001:db8::/64\n"
      "192.0.2.1\n");
  tmp.flush();
  tmp.close();

  libtorrent::ip_filter f;
  int applied = 0;
  ASSERT_TRUE(pfd::lt::loadIpFilterFile(tmp.fileName(), &f, &applied, nullptr));
  EXPECT_EQ(applied, 2);
  EXPECT_EQ(f.access(makeAddr("2001:db8::10")), libtorrent::ip_filter::blocked);
  EXPECT_EQ(f.access(makeAddr("192.0.2.1")), libtorrent::ip_filter::blocked);
  EXPECT_EQ(f.access(makeAddr("2001:db9::1")), 0u);
}

}  // namespace
