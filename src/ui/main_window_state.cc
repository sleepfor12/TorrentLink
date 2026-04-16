#include <QtCore/QDateTime>

#include "ui/main_window.h"

namespace pfd::ui {

void MainWindow::updateBottomStatus(int dhtNodes, qint64 downloadRate, qint64 uploadRate) {
  if (bottomStatus_ != nullptr) {
    bottomStatus_->setStatus(dhtNodes, downloadRate, uploadRate);
  }
  if (transferPage_ != nullptr) {
    transferPage_->addSpeedSample(downloadRate, uploadRate);
  }
}

void MainWindow::refreshRssItemsTaskProgress() {
  if (rssModulePage_ == nullptr)
    return;
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (now - lastRssProgressRefreshMs_ < 1000)
    return;
  lastRssProgressRefreshMs_ = now;
  rssModulePage_->refreshItemsTaskProgress();
}

void MainWindow::refreshTasks(const std::vector<pfd::core::TaskSnapshot>& snapshots) {
  snapshots_ = snapshots;
  if (transferPage_ != nullptr) {
    transferPage_->setSnapshots(snapshots);
    displayedSnapshots_ = transferPage_->displayedSnapshots();
  }
  refreshRssItemsTaskProgress();
}

pfd::base::TaskId MainWindow::selectedTaskId() const {
  if (transferPage_ != nullptr) {
    return transferPage_->selectedTaskId();
  }
  return {};
}

MainWindow::TaskFilter MainWindow::currentFilter() const {
  return transferPage_ != nullptr ? transferPage_->currentFilter() : TaskFilter::kAll;
}

MainWindow::SortKey MainWindow::currentSortKey() const {
  return transferPage_ != nullptr ? transferPage_->currentSortKey() : SortKey::kDownloadRate;
}

MainWindow::SortOrder MainWindow::currentSortOrder() const {
  return transferPage_ != nullptr ? transferPage_->currentSortOrder() : SortOrder::kDesc;
}

}  // namespace pfd::ui
