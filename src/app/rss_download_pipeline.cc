#include "app/rss_download_pipeline.h"

#include <QMetaObject>

#include <algorithm>

namespace pfd::app {

RssDownloadPipeline::RssDownloadPipeline(QObject* owner) : owner_(owner) {}

void RssDownloadPipeline::setMagnetMaxInFlight(int maxInFlight) {
  std::lock_guard<std::mutex> lk(magnetMu_);
  magnetMaxInFlight_ = std::max(1, maxInFlight);
}

void RssDownloadPipeline::setRssTorrentMaxInFlight(int maxInFlight) {
  std::lock_guard<std::mutex> lk(rssTorrentMu_);
  rssTorrentMaxInFlight_ = std::max(1, maxInFlight);
}

void RssDownloadPipeline::enqueueMagnet(const MagnetQueueItem& item, StartMagnetFn startFn) {
  {
    std::lock_guard<std::mutex> lk(magnetMu_);
    magnetQueue_.push_back(item);
  }
  schedulePumpMagnet(std::move(startFn));
}

void RssDownloadPipeline::enqueueRssTorrentUrl(const RssTorrentUrlQueueItem& item,
                                               StartRssTorrentFn startFn) {
  {
    std::lock_guard<std::mutex> lk(rssTorrentMu_);
    rssTorrentQueue_.push_back(item);
  }
  schedulePumpRssTorrent(std::move(startFn));
}

void RssDownloadPipeline::finishMagnet(StartMagnetFn startFn) {
  {
    std::lock_guard<std::mutex> lk(magnetMu_);
    magnetInFlight_ = std::max(0, magnetInFlight_ - 1);
  }
  schedulePumpMagnet(std::move(startFn));
}

void RssDownloadPipeline::finishRssTorrent(StartRssTorrentFn startFn) {
  {
    std::lock_guard<std::mutex> lk(rssTorrentMu_);
    rssTorrentInFlight_ = std::max(0, rssTorrentInFlight_ - 1);
  }
  schedulePumpRssTorrent(std::move(startFn));
}

void RssDownloadPipeline::schedulePumpMagnet(StartMagnetFn startFn) {
  bool shouldSchedule = false;
  {
    std::lock_guard<std::mutex> lk(magnetMu_);
    if (!magnetPumpScheduled_) {
      magnetPumpScheduled_ = true;
      shouldSchedule = true;
    }
  }
  if (shouldSchedule) {
    QMetaObject::invokeMethod(
        owner_, [this, fn = std::move(startFn)]() { pumpMagnetQueueOnUi(fn); }, Qt::QueuedConnection);
  }
}

void RssDownloadPipeline::schedulePumpRssTorrent(StartRssTorrentFn startFn) {
  bool shouldSchedule = false;
  {
    std::lock_guard<std::mutex> lk(rssTorrentMu_);
    if (!rssTorrentPumpScheduled_) {
      rssTorrentPumpScheduled_ = true;
      shouldSchedule = true;
    }
  }
  if (shouldSchedule) {
    QMetaObject::invokeMethod(owner_,
                              [this, fn = std::move(startFn)]() { pumpRssTorrentUrlQueueOnUi(fn); },
                              Qt::QueuedConnection);
  }
}

void RssDownloadPipeline::pumpMagnetQueueOnUi(StartMagnetFn startFn) {
  for (;;) {
    MagnetQueueItem item;
    {
      std::lock_guard<std::mutex> lk(magnetMu_);
      magnetPumpScheduled_ = false;
      if (magnetInFlight_ >= magnetMaxInFlight_ || magnetQueue_.empty()) {
        return;
      }
      item = magnetQueue_.front();
      magnetQueue_.pop_front();
      ++magnetInFlight_;
    }
    startFn(std::move(item));
  }
}

void RssDownloadPipeline::pumpRssTorrentUrlQueueOnUi(StartRssTorrentFn startFn) {
  for (;;) {
    RssTorrentUrlQueueItem item;
    {
      std::lock_guard<std::mutex> lk(rssTorrentMu_);
      rssTorrentPumpScheduled_ = false;
      if (rssTorrentInFlight_ >= rssTorrentMaxInFlight_ || rssTorrentQueue_.empty()) {
        return;
      }
      item = rssTorrentQueue_.front();
      rssTorrentQueue_.pop_front();
      ++rssTorrentInFlight_;
    }
    startFn(std::move(item));
  }
}

}  // namespace pfd::app
