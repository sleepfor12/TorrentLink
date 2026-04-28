#ifndef PFD_APP_RSS_DOWNLOAD_ORCHESTRATOR_H
#define PFD_APP_RSS_DOWNLOAD_ORCHESTRATOR_H

#include <QtCore/QString>

#include <atomic>
#include <functional>

#include "app/rss_download_pipeline.h"

class QObject;

namespace pfd::app {

class RssDownloadOrchestrator {
public:
  explicit RssDownloadOrchestrator(QObject* owner);

  using SettleFn =
      std::function<void(const pfd::core::rss::RssDownloadSettlement&, bool, const QString&)>;
  using StartMagnetFn = std::function<void(pfd::app::RssDownloadPipeline::MagnetQueueItem)>;
  using StartRssTorrentFn =
      std::function<void(pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem)>;

  void setMagnetMaxInFlight(int maxInFlight);
  void setHandlers(SettleFn settleFn, StartMagnetFn startMagnetFn,
                   StartRssTorrentFn startRssTorrentFn);

  void enqueueMagnet(const pfd::app::RssDownloadPipeline::MagnetQueueItem& item,
                     const std::atomic<bool>& shuttingDown);
  void enqueueRssTorrentUrl(const pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem& item,
                            const std::atomic<bool>& shuttingDown);
  void finishMagnet();
  void finishRssTorrent();

private:
  pfd::app::RssDownloadPipeline pipeline_;
  SettleFn settleFn_;
  StartMagnetFn startMagnetFn_;
  StartRssTorrentFn startRssTorrentFn_;
};

}  // namespace pfd::app

#endif  // PFD_APP_RSS_DOWNLOAD_ORCHESTRATOR_H
