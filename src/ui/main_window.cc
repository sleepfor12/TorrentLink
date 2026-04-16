#include "ui/main_window.h"

#include <QtGui/QCloseEvent>
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QTableWidget>

#include "core/config_service.h"
#include "core/rss/rss_service.h"
#include "ui/state/state_store.h"

namespace pfd::ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(QStringLiteral("极速下载"));
  resize(1180, 760);
  setupMenuBar();
  applyTheme();
  buildLayout();
  bindSignals();
  loadSettingsToUi();
  loadUiState();
  qApp->installEventFilter(this);
}

void MainWindow::setRssService(pfd::core::rss::RssService* service) {
  rssService_ = service;
  if (rssModulePage_) {
    rssModulePage_->setService(service);
  }
}

void MainWindow::setRssItemsUiHelpers(std::function<void(const QString&)> appendItemsLog,
                                      std::function<QString()> defaultSaveRoot,
                                      std::function<std::vector<pfd::core::TaskSnapshot>()> taskSnapshots) {
  if (rssModulePage_) {
    rssModulePage_->setItemsPageUiHelpers(std::move(appendItemsLog), std::move(defaultSaveRoot),
                                          std::move(taskSnapshots));
  }
}

void MainWindow::showTransferTab() {
  if (tabs_ != nullptr) {
    tabs_->setCurrentIndex(0);
  }
}

void MainWindow::refreshRssDataViews() {
  if (!rssModulePage_) {
    return;
  }
  if (!rssRefreshDebounce_) {
    rssRefreshDebounce_ = new QTimer(this);
    rssRefreshDebounce_->setSingleShot(true);
    rssRefreshDebounce_->setInterval(80);
    connect(rssRefreshDebounce_, &QTimer::timeout, this, [this]() {
      if (rssModulePage_) {
        rssModulePage_->refreshDataViews();
      }
    });
  }
  rssRefreshDebounce_->start();
}

void MainWindow::notifyTaskAlreadyInList(const QString& displayName) {
  const QString body =
      displayName.isEmpty()
          ? QStringLiteral("该任务已在下载列表中，无需重复添加。")
          : QStringLiteral("任务「%1」已在下载列表中，无需重复添加。").arg(displayName);
  QMessageBox::information(this, QStringLiteral("提示"), body);
  appendLog(QStringLiteral("[提示] %1").arg(body));
}

void MainWindow::closeEvent(QCloseEvent* event) {
  saveUiState();
  const bool wantQuit =
      forceQuit_ || !QSystemTrayIcon::isSystemTrayAvailable() ||
      pfd::core::ConfigService::loadAppSettings().close_behavior == QStringLiteral("quit");
  if (wantQuit) {
    QMainWindow::closeEvent(event);
  } else {
    hide();
    event->ignore();
  }
}

void MainWindow::loadUiState() {
  const auto st = StateStore::loadMainWindowState();
  if (!st.geometry.isEmpty()) {
    restoreGeometry(st.geometry);
  }
  if (!st.state.isEmpty()) {
    restoreState(st.state);
  }
  if (tabs_ != nullptr) {
    // UX: 每次启动默认进入“传输”页（索引 0）
    tabs_->setCurrentIndex(0);
  }
  if (transferPage_ != nullptr) {
    transferPage_->restoreTaskHeaderState(st.taskHeaderState);
    transferPage_->setSortKey(static_cast<pfd::ui::TransferPage::SortKey>(std::max(0, st.sortKey)));
    transferPage_->setSortOrder(
        static_cast<pfd::ui::TransferPage::SortOrder>(std::max(0, st.sortOrder)));
    // UX: 每次启动默认“全部”状态列表
    transferPage_->setFilter(pfd::ui::TransferPage::TaskFilter::kAll);
  }
}

void MainWindow::saveUiState() const {
  MainWindowUiState st;
  st.geometry = saveGeometry();
  st.state = saveState();
  if (transferPage_ != nullptr) {
    st.taskHeaderState = transferPage_->taskHeaderSaveState();
    st.sortKey = static_cast<int>(transferPage_->currentSortKey());
    st.sortOrder = static_cast<int>(transferPage_->currentSortOrder());
  }
  StateStore::saveMainWindowState(st);
}

}  // namespace pfd::ui
