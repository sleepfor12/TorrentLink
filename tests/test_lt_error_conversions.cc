#include <cerrno>
#include <gtest/gtest.h>
#include <system_error>

#include "lt/error_conversions.h"

namespace {

TEST(LtErrorConversions, AddTorrentMapsToAddTorrentFailed) {
  const auto err =
      pfd::lt::fromLibtorrentError(101, QStringLiteral("duplicate torrent"), QStringLiteral("add"),
                                   pfd::lt::LtErrorHint::kAddTorrent);
  EXPECT_EQ(err.code(), pfd::base::ErrorCode::kAddTorrentFailed);
  EXPECT_EQ(err.domain(), pfd::base::ErrorDomain::kLibtorrent);
  EXPECT_EQ(err.rawCode(), 101);
}

TEST(LtErrorConversions, StorageNoSpaceMapsToDiskFull) {
  const auto err =
      pfd::lt::fromLibtorrentError(ENOSPC, QStringLiteral("no space left"),
                                   QStringLiteral("write piece"), pfd::lt::LtErrorHint::kStorage);
  EXPECT_EQ(err.code(), pfd::base::ErrorCode::kDiskFull);
  EXPECT_EQ(err.domain(), pfd::base::ErrorDomain::kLibtorrent);
  EXPECT_EQ(err.rawCode(), ENOSPC);
}

TEST(LtErrorConversions, EmptyMessageAndZeroCodeMeansSuccess) {
  const auto err =
      pfd::lt::fromLibtorrentError(0, QString(), QString(), pfd::lt::LtErrorHint::kGeneric);
  EXPECT_TRUE(err.isOk());
  EXPECT_EQ(err.code(), pfd::base::ErrorCode::kSuccess);
}

TEST(LtErrorConversions, MetadataHintMapsToMetadataTimeout) {
  const auto err =
      pfd::lt::fromLibtorrentError(110, QStringLiteral("metadata timeout"),
                                   QStringLiteral("metadata"), pfd::lt::LtErrorHint::kMetadata);
  EXPECT_EQ(err.code(), pfd::base::ErrorCode::kMetadataTimeout);
  EXPECT_EQ(err.domain(), pfd::base::ErrorDomain::kLibtorrent);
}

TEST(LtErrorConversions, NetworkTimeoutMapsToMetadataTimeoutFallback) {
  const auto err =
      pfd::lt::fromLibtorrentError(ETIMEDOUT, QStringLiteral("network timeout"),
                                   QStringLiteral("network"), pfd::lt::LtErrorHint::kNetwork);
  EXPECT_EQ(err.code(), pfd::base::ErrorCode::kMetadataTimeout);
}

TEST(LtErrorConversions, StdErrorOverloadWorks) {
  const std::error_code ec(ENOENT, std::generic_category());
  const auto err =
      pfd::lt::fromLibtorrentError(ec, QStringLiteral("announce"), pfd::lt::LtErrorHint::kGeneric);
  EXPECT_EQ(err.code(), pfd::base::ErrorCode::kUnknown);
  EXPECT_EQ(err.domain(), pfd::base::ErrorDomain::kLibtorrent);
  EXPECT_EQ(err.rawCode(), ENOENT);
}

}  // namespace
