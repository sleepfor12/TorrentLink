#include <libtorrent/torrent_status.hpp>

#include <gtest/gtest.h>

#include "lt/task_alert_adapter.h"

namespace {

TEST(LtTaskAlertAdapter, NullAlertReturnsNullopt) {
  EXPECT_FALSE(pfd::lt::adaptAlertToView(nullptr).has_value());
}

TEST(LtTaskAlertAdapter, NullAlertReturnsEmptyBatch) {
  EXPECT_TRUE(pfd::lt::adaptAlertToViews(nullptr).empty());
}

TEST(LtTaskAlertAdapter, TorrentStatusMapsToProgressView) {
  libtorrent::torrent_status st;
  st.state = libtorrent::torrent_status::downloading;
  st.total_wanted = 1024;
  st.total_done = 256;
  st.total_upload = 64;
  st.download_rate = 128;
  st.upload_rate = 32;
  st.num_seeds = 7;
  st.num_peers = 21;
  st.distributed_copies = 1.5f;

  const auto view = pfd::lt::adaptTorrentStatusToProgressView(st);
  EXPECT_EQ(view.type, pfd::lt::LtAlertView::Type::kTaskProgress);
  EXPECT_FALSE(view.taskId.isNull());
  EXPECT_EQ(view.status, pfd::base::TaskStatus::kDownloading);
  EXPECT_EQ(view.totalBytes, 1024);
  EXPECT_EQ(view.downloadedBytes, 256);
  EXPECT_EQ(view.uploadedBytes, 64);
  EXPECT_EQ(view.downloadRate, 128);
  EXPECT_EQ(view.uploadRate, 32);
  EXPECT_EQ(view.seeders, 7);
  EXPECT_EQ(view.users, 21);
  EXPECT_DOUBLE_EQ(view.availability, 1.5);
}

TEST(LtTaskAlertAdapter, TorrentStatusStateMapsToSeedingStatus) {
  libtorrent::torrent_status st;
  st.state = libtorrent::torrent_status::seeding;

  const auto view = pfd::lt::adaptTorrentStatusToProgressView(st);
  EXPECT_EQ(view.status, pfd::base::TaskStatus::kSeeding);
}

TEST(LtTaskAlertAdapter, PausedFlagMapsToPausedStatus) {
  libtorrent::torrent_status st;
  st.state = libtorrent::torrent_status::downloading;
  st.flags = libtorrent::torrent_flags::paused;

  const auto view = pfd::lt::adaptTorrentStatusToProgressView(st);
  EXPECT_EQ(view.status, pfd::base::TaskStatus::kPaused);
}

}  // namespace
