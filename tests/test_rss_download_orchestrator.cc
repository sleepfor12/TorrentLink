#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QObject>
#include <QtCore/QTimer>

#include <atomic>
#include <gtest/gtest.h>

#include "app/rss_download_orchestrator.h"

namespace {

void spinEvents() {
  QTimer timer;
  timer.setSingleShot(true);
  timer.start(10);
  while (timer.isActive()) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
  }
}

}  // namespace

TEST(RssDownloadOrchestratorTest, ShutdownMagnetSettlesWithoutStarting) {
  QObject owner;
  pfd::app::RssDownloadOrchestrator orchestrator(&owner);
  std::atomic<bool> started{false};
  std::atomic<bool> settled{false};
  orchestrator.setHandlers(
      [&settled](const pfd::core::rss::RssDownloadSettlement&, bool ok, const QString&) {
        settled.store(!ok);
      },
      [&started](pfd::app::RssDownloadPipeline::MagnetQueueItem) { started.store(true); },
      [](pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem) {});

  std::atomic<bool> shuttingDown{true};
  pfd::app::RssDownloadPipeline::MagnetQueueItem item;
  item.uri = QStringLiteral("magnet:?xt=urn:btih:123");
  item.rssSettlement.item_id = QStringLiteral("item1");
  orchestrator.enqueueMagnet(item, shuttingDown);
  spinEvents();

  EXPECT_FALSE(started.load());
  EXPECT_TRUE(settled.load());
}

TEST(RssDownloadOrchestratorTest, ActiveMagnetStartsHandler) {
  QObject owner;
  pfd::app::RssDownloadOrchestrator orchestrator(&owner);
  std::atomic<bool> started{false};
  orchestrator.setHandlers(
      [](const pfd::core::rss::RssDownloadSettlement&, bool, const QString&) {},
      [&started, &orchestrator](pfd::app::RssDownloadPipeline::MagnetQueueItem) {
        started.store(true);
        orchestrator.finishMagnet();
      },
      [](pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem) {});

  std::atomic<bool> shuttingDown{false};
  pfd::app::RssDownloadPipeline::MagnetQueueItem item;
  item.uri = QStringLiteral("magnet:?xt=urn:btih:456");
  orchestrator.enqueueMagnet(item, shuttingDown);
  spinEvents();

  EXPECT_TRUE(started.load());
}

TEST(RssDownloadOrchestratorTest, ShutdownTorrentUrlSettlesWithoutStarting) {
  QObject owner;
  pfd::app::RssDownloadOrchestrator orchestrator(&owner);
  std::atomic<bool> started{false};
  std::atomic<bool> settled{false};
  orchestrator.setHandlers(
      [&settled](const pfd::core::rss::RssDownloadSettlement&, bool ok, const QString&) {
        settled.store(!ok);
      },
      [](pfd::app::RssDownloadPipeline::MagnetQueueItem) {},
      [&started](pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem) { started.store(true); });

  std::atomic<bool> shuttingDown{true};
  pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem item;
  item.url = QStringLiteral("https://example.com/a.torrent");
  item.rssSettlement.item_id = QStringLiteral("item2");
  orchestrator.enqueueRssTorrentUrl(item, shuttingDown);
  spinEvents();

  EXPECT_FALSE(started.load());
  EXPECT_TRUE(settled.load());
}
