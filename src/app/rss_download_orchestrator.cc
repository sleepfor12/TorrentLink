#include "app/rss_download_orchestrator.h"

namespace pfd::app {

RssDownloadOrchestrator::RssDownloadOrchestrator(QObject* owner) : pipeline_(owner) {}

void RssDownloadOrchestrator::setMagnetMaxInFlight(int maxInFlight) {
  pipeline_.setMagnetMaxInFlight(maxInFlight);
}

void RssDownloadOrchestrator::setHandlers(SettleFn settleFn, StartMagnetFn startMagnetFn,
                                          StartRssTorrentFn startRssTorrentFn) {
  settleFn_ = std::move(settleFn);
  startMagnetFn_ = std::move(startMagnetFn);
  startRssTorrentFn_ = std::move(startRssTorrentFn);
}

void RssDownloadOrchestrator::enqueueMagnet(
    const pfd::app::RssDownloadPipeline::MagnetQueueItem& item, const std::atomic<bool>& shuttingDown) {
  pipeline_.enqueueMagnet(item, [this, &shuttingDown](pfd::app::RssDownloadPipeline::MagnetQueueItem in) {
    if (!shuttingDown.load() && startMagnetFn_) {
      startMagnetFn_(std::move(in));
      return;
    }
    if (settleFn_) {
      settleFn_(in.rssSettlement, false, {});
    }
    pipeline_.finishMagnet([this](pfd::app::RssDownloadPipeline::MagnetQueueItem next) {
      if (startMagnetFn_) {
        startMagnetFn_(std::move(next));
      }
    });
  });
}

void RssDownloadOrchestrator::enqueueRssTorrentUrl(
    const pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem& item,
    const std::atomic<bool>& shuttingDown) {
  pipeline_.enqueueRssTorrentUrl(item, [this, &shuttingDown](
                                           pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem in) {
    if (!shuttingDown.load() && startRssTorrentFn_) {
      startRssTorrentFn_(std::move(in));
      return;
    }
    if (settleFn_) {
      settleFn_(in.rssSettlement, false, {});
    }
    pipeline_.finishRssTorrent([this](pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem next) {
      if (startRssTorrentFn_) {
        startRssTorrentFn_(std::move(next));
      }
    });
  });
}

void RssDownloadOrchestrator::finishMagnet() {
  pipeline_.finishMagnet([this](pfd::app::RssDownloadPipeline::MagnetQueueItem next) {
    if (startMagnetFn_) {
      startMagnetFn_(std::move(next));
    }
  });
}

void RssDownloadOrchestrator::finishRssTorrent() {
  pipeline_.finishRssTorrent([this](pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem next) {
    if (startRssTorrentFn_) {
      startRssTorrentFn_(std::move(next));
    }
  });
}

}  // namespace pfd::app
