#ifndef PFD_APP_RSS_DOWNLOAD_PIPELINE_H
#define PFD_APP_RSS_DOWNLOAD_PIPELINE_H

#include <QtCore/QString>

#include <deque>
#include <functional>
#include <mutex>

#include "core/rss/rss_service.h"

class QObject;

namespace pfd::app {

class RssDownloadPipeline {
public:
  struct MagnetQueueItem {
    QString uri;
    QString savePath;
    pfd::core::rss::RssDownloadSettlement rssSettlement;
    bool skipInteractiveAdd{false};
    QString category;
    QString tagsCsv;
  };

  struct RssTorrentUrlQueueItem {
    QString url;
    QString savePath;
    QString referer;
    pfd::core::rss::RssDownloadSettlement rssSettlement;
  };

  using StartMagnetFn = std::function<void(MagnetQueueItem)>;
  using StartRssTorrentFn = std::function<void(RssTorrentUrlQueueItem)>;

  explicit RssDownloadPipeline(QObject* owner);

  void setMagnetMaxInFlight(int maxInFlight);
  void setRssTorrentMaxInFlight(int maxInFlight);

  void enqueueMagnet(const MagnetQueueItem& item, StartMagnetFn startFn);
  void enqueueRssTorrentUrl(const RssTorrentUrlQueueItem& item, StartRssTorrentFn startFn);

  void finishMagnet(StartMagnetFn startFn);
  void finishRssTorrent(StartRssTorrentFn startFn);

private:
  void schedulePumpMagnet(StartMagnetFn startFn);
  void schedulePumpRssTorrent(StartRssTorrentFn startFn);
  void pumpMagnetQueueOnUi(StartMagnetFn startFn);
  void pumpRssTorrentUrlQueueOnUi(StartRssTorrentFn startFn);

  QObject* owner_{nullptr};

  std::mutex magnetMu_;
  std::deque<MagnetQueueItem> magnetQueue_;
  int magnetInFlight_{0};
  int magnetMaxInFlight_{8};
  bool magnetPumpScheduled_{false};

  std::mutex rssTorrentMu_;
  std::deque<RssTorrentUrlQueueItem> rssTorrentQueue_;
  int rssTorrentInFlight_{0};
  int rssTorrentMaxInFlight_{8};
  bool rssTorrentPumpScheduled_{false};
};

}  // namespace pfd::app

#endif  // PFD_APP_RSS_DOWNLOAD_PIPELINE_H
