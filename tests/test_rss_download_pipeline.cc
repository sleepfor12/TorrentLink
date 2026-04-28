#include <QtCore/QEventLoop>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QCoreApplication>

#include <gtest/gtest.h>

#include <vector>

#include "app/rss_download_pipeline.h"

namespace {

void spinEvents(int ms = 20) {
  QTimer timer;
  timer.setSingleShot(true);
  timer.start(ms);
  while (timer.isActive()) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
  }
}

}  // namespace

TEST(RssDownloadPipelineTest, MagnetQueueRespectsMaxInFlightAndFIFO) {
  QObject owner;
  pfd::app::RssDownloadPipeline pipeline(&owner);
  pipeline.setMagnetMaxInFlight(1);
  std::vector<QString> started;
  auto startFn = [&](pfd::app::RssDownloadPipeline::MagnetQueueItem item) {
    started.push_back(item.uri);
  };

  pipeline.enqueueMagnet({QStringLiteral("m1"), {}, {}, false, {}, {}}, startFn);
  pipeline.enqueueMagnet({QStringLiteral("m2"), {}, {}, false, {}, {}}, startFn);
  pipeline.enqueueMagnet({QStringLiteral("m3"), {}, {}, false, {}, {}}, startFn);
  spinEvents();
  ASSERT_EQ(started.size(), 1u);
  EXPECT_EQ(started[0], QStringLiteral("m1"));

  pipeline.finishMagnet(startFn);
  spinEvents();
  ASSERT_EQ(started.size(), 2u);
  EXPECT_EQ(started[1], QStringLiteral("m2"));

  pipeline.finishMagnet(startFn);
  spinEvents();
  ASSERT_EQ(started.size(), 3u);
  EXPECT_EQ(started[2], QStringLiteral("m3"));
}

TEST(RssDownloadPipelineTest, TorrentUrlQueueRespectsMaxInFlightAndFIFO) {
  QObject owner;
  pfd::app::RssDownloadPipeline pipeline(&owner);
  pipeline.setRssTorrentMaxInFlight(1);
  std::vector<QString> started;
  auto startFn = [&](pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem item) {
    started.push_back(item.url);
  };

  pipeline.enqueueRssTorrentUrl({QStringLiteral("u1"), {}, {}, {}}, startFn);
  pipeline.enqueueRssTorrentUrl({QStringLiteral("u2"), {}, {}, {}}, startFn);
  spinEvents();
  ASSERT_EQ(started.size(), 1u);
  EXPECT_EQ(started[0], QStringLiteral("u1"));

  pipeline.finishRssTorrent(startFn);
  spinEvents();
  ASSERT_EQ(started.size(), 2u);
  EXPECT_EQ(started[1], QStringLiteral("u2"));
}
