#include <QtCore/QDateTime>
#include <QtCore/QSignalBlocker>
#include <QtGui/QAction>

#include "ui/main_window.h"

namespace pfd::ui {

void MainWindow::updateBottomStatus(int dhtNodes, qint64 downloadRate, qint64 uploadRate) {
  if (bottomStatusBarVisible_ && bottomStatus_ != nullptr) {
    bottomStatus_->setStatus(dhtNodes, downloadRate, uploadRate);
  }
  if (transferDetailPanelVisible_ && transferPage_ != nullptr) {
    transferPage_->addSpeedSample(downloadRate, uploadRate);
  }
}

void MainWindow::setBottomStatusBarVisible(bool visible) {
  if (bottomStatusBarVisible_ == visible) {
    return;
  }
  bottomStatusBarVisible_ = visible;
  applyTransferBottomPanelsVisible();
}

bool MainWindow::bottomStatusBarVisible() const {
  return bottomStatusBarVisible_;
}

void MainWindow::setTransferDetailPanelVisible(bool visible) {
  if (transferDetailPanelVisible_ == visible) {
    return;
  }
  transferDetailPanelVisible_ = visible;
  applyTransferBottomPanelsVisible();
}

bool MainWindow::transferDetailPanelVisible() const {
  return transferDetailPanelVisible_;
}

void MainWindow::applyTransferBottomPanelsVisible() {
  if (showBottomStatusBarAction_ != nullptr) {
    const QSignalBlocker blocker(showBottomStatusBarAction_);
    showBottomStatusBarAction_->setChecked(bottomStatusBarVisible_);
  }
  if (showTransferDetailPanelAction_ != nullptr) {
    const QSignalBlocker blocker(showTransferDetailPanelAction_);
    showTransferDetailPanelAction_->setChecked(transferDetailPanelVisible_);
  }
  if (bottomStatus_ != nullptr) {
    bottomStatus_->setVisible(bottomStatusBarVisible_);
  }
  if (transferPage_ != nullptr) {
    transferPage_->setDetailPanelVisible(transferDetailPanelVisible_);
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
