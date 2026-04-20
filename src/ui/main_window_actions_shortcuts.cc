#include <QtCore/QDateTime>
#include <QtCore/QEvent>
#include <QtCore/QFileInfo>
#include <QtCore/QPoint>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtCore/QUuid>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtGui/QMouseEvent>
#include <QtGui/QShortcut>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

#include "base/types.h"
#include "core/config_service.h"
#include "core/logger.h"
#include "ui/main_window.h"

namespace pfd::ui {

void MainWindow::bindSignals() {
  if (openTorrentFileAction_ != nullptr) {
    connect(openTorrentFileAction_, &QAction::triggered, this,
            [this]() { openTorrentFileFromMenu(); });
  }
  if (openTorrentLinksAction_ != nullptr) {
    connect(openTorrentLinksAction_, &QAction::triggered, this,
            [this]() { openTorrentLinksFromMenu(); });
  }
  if (exitAction_ != nullptr) {
    connect(exitAction_, &QAction::triggered, this, [this]() {
      forceQuit_ = true;
      close();
    });
  }
  if (preferencesAction_ != nullptr) {
    connect(preferencesAction_, &QAction::triggered, this, [this]() { openPreferences(); });
  }
  if (logCenterAction_ != nullptr) {
    connect(logCenterAction_, &QAction::triggered, this, [this]() { openLogCenter(); });
  }
  if (downloadSettingsAction_ != nullptr) {
    connect(downloadSettingsAction_, &QAction::triggered, this, [this]() { openPreferences(); });
  }
  if (showLogAction_ != nullptr) {
    connect(showLogAction_, &QAction::triggered, this, [this]() { openLogCenter(); });
  }
  if (refreshListAction_ != nullptr) {
    connect(refreshListAction_, &QAction::triggered, this, [this]() { refreshTasks(snapshots_); });
  }
  if (themeAction_ != nullptr) {
    connect(themeAction_, &QAction::triggered, this, [this]() {
      QMessageBox::information(this, QStringLiteral("界面主题"),
                               QStringLiteral("界面主题切换功能开发中。"));
    });
  }
  if (helpAction_ != nullptr) {
    connect(helpAction_, &QAction::triggered, this, [this]() {
      QMessageBox::information(
          this, QStringLiteral("使用说明"),
          QStringLiteral("<h3>P2P 文件下载系统 — 使用说明</h3>"
                         "<ul>"
                         "<li>通过 <b>文件</b> 菜单添加 Torrent 文件或磁力链接</li>"
                         "<li>在 <b>传输</b> 页查看和管理下载任务</li>"
                         "<li>右键任务可暂停、删除、修改 Tracker 等</li>"
                         "<li>使用 <b>RSS 订阅</b> 页自动追踪新内容</li>"
                         "<li>使用 <b>搜索</b> 页在本地历史和 RSS 条目中检索</li>"
                         "<li>关闭窗口后程序将最小化到系统托盘</li>"
                         "</ul>"));
    });
  }
  if (aboutAction_ != nullptr) {
    connect(aboutAction_, &QAction::triggered, this, [this]() {
      QMessageBox::about(
          this, QStringLiteral("关于"),
          QStringLiteral("<h3>P2P 文件下载系统</h3>"
                         "<p>版本: 1.0.0</p>"
                         "<p>基于 <b>libtorrent-rasterbar 2.x</b> 和 <b>Qt %1</b> 构建。</p>"
                         "<p>许可证: MIT</p>")
              .arg(QString::fromLatin1(qVersion())));
    });
  }

  auto* selectAllShortcut = new QShortcut(QKeySequence::SelectAll, this);
  connect(selectAllShortcut, &QShortcut::activated, this, [this]() {
    if (taskTable_ != nullptr) {
      taskTable_->selectAll();
      if (taskTable_->rowCount() > 0 && taskTable_->selectionModel() != nullptr) {
        const QModelIndex lastIdx = taskTable_->model()->index(taskTable_->rowCount() - 1, 0);
        taskTable_->selectionModel()->setCurrentIndex(lastIdx, QItemSelectionModel::NoUpdate);
      }
      appendLog(QStringLiteral("已全选所有任务（Ctrl+A）。"));
    }
  });

  if (transferPage_ != nullptr) {
    connect(transferPage_, &pfd::ui::TransferPage::filterChanged, this,
            [this]() { refreshTasks(snapshots_); });
    connect(transferPage_, &pfd::ui::TransferPage::sortChanged, this,
            [this]() { refreshTasks(snapshots_); });
  }

  if (taskTable_ != nullptr) {
    connect(taskTable_, &QTableWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) { showTaskContextMenu(pos); });
  }
}

void MainWindow::openTorrentLinksFromMenu() {
  QDialog dlg(this);
  dlg.setWindowTitle(QStringLiteral("打开 Torrent 链接"));
  dlg.setModal(true);
  dlg.resize(720, 420);
  auto* layout = new QVBoxLayout(&dlg);
  layout->addWidget(new QLabel(
      QStringLiteral("每行一个链接（磁力链接 magnet:?xt=...），可一次添加多个。"), &dlg));
  auto* edit = new QTextEdit(&dlg);
  edit->setPlaceholderText(QStringLiteral("magnet:?xt=urn:btih:...\nmagnet:?xt=urn:btih:..."));
  layout->addWidget(edit, 1);
  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
  layout->addWidget(buttons);

  if (dlg.exec() != QDialog::Accepted) {
    appendLog(QStringLiteral("已取消打开 Torrent 链接。"));
    return;
  }
  const QStringList lines = edit->toPlainText().split('\n', Qt::SkipEmptyParts);
  int submitted = 0;
  for (const auto& line : lines) {
    const QString uri = line.trimmed();
    if (uri.isEmpty()) {
      continue;
    }
    if (onAddMagnet_) {
      onAddMagnet_(uri, selectedSavePath());
      ++submitted;
    }
  }
  appendLog(QStringLiteral("已提交 Torrent 链接：%1 条").arg(submitted));
}

void MainWindow::setOnAddMagnet(std::function<void(const QString&, const QString&)> onAddMagnet) {
  onAddMagnet_ = std::move(onAddMagnet);
}

void MainWindow::setOnAddTorrentFile(
    std::function<void(const QString&, const QString&)> onAddTorrentFile) {
  onAddTorrentFile_ = std::move(onAddTorrentFile);
}

void MainWindow::setOnAddTorrentFileRequest(
    std::function<void(const AddTorrentFileRequest&)> onAddTorrentFileRequest) {
  onAddTorrentFileRequest_ = std::move(onAddTorrentFileRequest);
}

void MainWindow::setOnPauseTask(std::function<void(const pfd::base::TaskId&)> onPauseTask) {
  onPauseTask_ = std::move(onPauseTask);
}

void MainWindow::setOnResumeTask(std::function<void(const pfd::base::TaskId&)> onResumeTask) {
  onResumeTask_ = std::move(onResumeTask);
}

void MainWindow::setOnRemoveTask(std::function<void(const pfd::base::TaskId&, bool)> onRemoveTask) {
  onRemoveTask_ = std::move(onRemoveTask);
}

void MainWindow::setOnMoveTask(
    std::function<void(const pfd::base::TaskId&, const QString&)> onMoveTask) {
  onMoveTask_ = std::move(onMoveTask);
}

void MainWindow::setOnRenameTask(
    std::function<void(const pfd::base::TaskId&, const QString&)> onRenameTask) {
  onRenameTask_ = std::move(onRenameTask);
}

void MainWindow::setOnEditTrackers(
    std::function<void(const pfd::base::TaskId&, const QStringList&)> onEditTrackers) {
  onEditTrackers_ = std::move(onEditTrackers);
}

void MainWindow::setOnQueryTaskTrackers(
    std::function<QStringList(const pfd::base::TaskId&)> onQueryTaskTrackers) {
  onQueryTaskTrackers_ = std::move(onQueryTaskTrackers);
}

void MainWindow::setOnCategoryChanged(
    std::function<void(const pfd::base::TaskId&, const QString&)> onCategoryChanged) {
  onCategoryChanged_ = std::move(onCategoryChanged);
}

void MainWindow::setOnTagsChanged(
    std::function<void(const pfd::base::TaskId&, const QStringList&)> onTagsChanged) {
  onTagsChanged_ = std::move(onTagsChanged);
}

void MainWindow::setOnDefaultTrackersChanged(
    std::function<void(bool, const QStringList&)> onDefaultTrackersChanged) {
  onDefaultTrackersChanged_ = std::move(onDefaultTrackersChanged);
}

void MainWindow::setOnQueryDefaultTrackers(std::function<QStringList()> onQueryDefaultTrackers) {
  onQueryDefaultTrackers_ = std::move(onQueryDefaultTrackers);
}

void MainWindow::setOnQueryDefaultTrackerEnabled(
    std::function<bool()> onQueryDefaultTrackerEnabled) {
  onQueryDefaultTrackerEnabled_ = std::move(onQueryDefaultTrackerEnabled);
}

void MainWindow::setOnSeedPolicyChanged(std::function<void(bool, double)> onSeedPolicyChanged) {
  onSeedPolicyChanged_ = std::move(onSeedPolicyChanged);
}

void MainWindow::setOnSetTaskConnectionsLimit(
    std::function<void(const pfd::base::TaskId&, int)> onSetTaskConnectionsLimit) {
  onSetTaskConnectionsLimit_ = std::move(onSetTaskConnectionsLimit);
}

void MainWindow::setOnForceStartTask(
    std::function<void(const pfd::base::TaskId&)> onForceStartTask) {
  onForceStartTask_ = std::move(onForceStartTask);
}

void MainWindow::setOnForceRecheckTask(
    std::function<void(const pfd::base::TaskId&)> onForceRecheckTask) {
  onForceRecheckTask_ = std::move(onForceRecheckTask);
}

void MainWindow::setOnForceReannounceTask(
    std::function<void(const pfd::base::TaskId&)> onForceReannounceTask) {
  onForceReannounceTask_ = std::move(onForceReannounceTask);
}

void MainWindow::setOnSetSequentialDownload(
    std::function<void(const pfd::base::TaskId&, bool)> onSetSequentialDownload) {
  onSetSequentialDownload_ = std::move(onSetSequentialDownload);
}

void MainWindow::setOnSetAutoManagedTask(
    std::function<void(const pfd::base::TaskId&, bool)> onSetAutoManagedTask) {
  onSetAutoManagedTask_ = std::move(onSetAutoManagedTask);
}

void MainWindow::setOnFirstLastPiecePriority(
    std::function<void(const pfd::base::TaskId&, bool)> onFirstLastPiecePriority) {
  onFirstLastPiecePriority_ = std::move(onFirstLastPiecePriority);
}

void MainWindow::setOnPreviewFile(std::function<void(const pfd::base::TaskId&)> onPreviewFile) {
  onPreviewFile_ = std::move(onPreviewFile);
}

void MainWindow::setOnQueuePositionUp(std::function<void(const pfd::base::TaskId&)> cb) {
  onQueuePositionUp_ = std::move(cb);
}

void MainWindow::setOnQueuePositionDown(std::function<void(const pfd::base::TaskId&)> cb) {
  onQueuePositionDown_ = std::move(cb);
}

void MainWindow::setOnQueuePositionTop(std::function<void(const pfd::base::TaskId&)> cb) {
  onQueuePositionTop_ = std::move(cb);
}

void MainWindow::setOnQueuePositionBottom(std::function<void(const pfd::base::TaskId&)> cb) {
  onQueuePositionBottom_ = std::move(cb);
}

void MainWindow::setOnQueryCopyPayload(
    std::function<CopyPayload(const pfd::base::TaskId&)> onQueryCopyPayload) {
  onQueryCopyPayload_ = std::move(onQueryCopyPayload);
}

void MainWindow::setOnQueryTaskFiles(
    std::function<std::vector<pfd::core::TaskFileEntryDto>(const pfd::base::TaskId&)> cb) {
  onQueryTaskFiles_ = std::move(cb);
  syncContentHandlers();
}

void MainWindow::setOnSetTaskFilePriority(
    std::function<void(const pfd::base::TaskId&, const std::vector<int>&,
                       pfd::core::TaskFilePriorityLevel)>
        cb) {
  onSetTaskFilePriority_ = std::move(cb);
  syncContentHandlers();
}

void MainWindow::setOnRenameTaskFileOrFolder(
    std::function<void(const pfd::base::TaskId&, const QString&, const QString&)> cb) {
  onRenameTaskFileOrFolder_ = std::move(cb);
  syncContentHandlers();
}

void MainWindow::setOnQueryTaskTrackerSnapshot(
    std::function<pfd::core::TaskTrackerSnapshotDto(const pfd::base::TaskId&)> cb) {
  onQueryTaskTrackerSnapshot_ = std::move(cb);
  syncTrackerHandlers();
}

void MainWindow::setOnAddTaskTracker(
    std::function<void(const pfd::base::TaskId&, const QString&)> cb) {
  onAddTaskTracker_ = std::move(cb);
  syncTrackerHandlers();
}

void MainWindow::setOnEditTaskTracker(
    std::function<void(const pfd::base::TaskId&, const QString&, const QString&)> cb) {
  onEditTaskTracker_ = std::move(cb);
  syncTrackerHandlers();
}

void MainWindow::setOnRemoveTaskTracker(
    std::function<void(const pfd::base::TaskId&, const QString&)> cb) {
  onRemoveTaskTracker_ = std::move(cb);
  syncTrackerHandlers();
}

void MainWindow::setOnForceReannounceTracker(
    std::function<void(const pfd::base::TaskId&, const QString&)> cb) {
  onForceReannounceTracker_ = std::move(cb);
  syncTrackerHandlers();
}

void MainWindow::setOnForceReannounceAllTrackers(std::function<void(const pfd::base::TaskId&)> cb) {
  onForceReannounceAllTrackers_ = std::move(cb);
  syncTrackerHandlers();
}

void MainWindow::setOnQueryTaskPeers(
    std::function<std::vector<pfd::core::TaskPeerDto>(const pfd::base::TaskId&)> cb) {
  onQueryTaskPeers_ = std::move(cb);
  if (transferPage_ != nullptr) {
    transferPage_->setPeerHandlers(onQueryTaskPeers_);
  }
}

void MainWindow::setOnQueryTaskWebSeeds(
    std::function<std::vector<pfd::core::TaskWebSeedDto>(const pfd::base::TaskId&)> cb) {
  onQueryTaskWebSeeds_ = std::move(cb);
  if (transferPage_ != nullptr) {
    transferPage_->setHttpSourceHandlers(onQueryTaskWebSeeds_);
  }
}

void MainWindow::setSearchDataSources(SearchTab::QuerySnapshotsFn snapshotsFn,
                                      SearchTab::QueryRssItemsFn rssItemsFn) {
  if (searchTab_ != nullptr) {
    searchTab_->setQuerySnapshotsFn(std::move(snapshotsFn));
    searchTab_->setQueryRssItemsFn(std::move(rssItemsFn));
  }
}

void MainWindow::syncContentHandlers() {
  if (transferPage_ == nullptr)
    return;
  transferPage_->setContentHandlers(onQueryTaskFiles_, onSetTaskFilePriority_,
                                    onRenameTaskFileOrFolder_);
}

void MainWindow::syncTrackerHandlers() {
  if (transferPage_ == nullptr)
    return;
  transferPage_->setTrackerHandlers(onQueryTaskTrackerSnapshot_, onAddTaskTracker_,
                                    onEditTaskTracker_, onRemoveTaskTracker_,
                                    onForceReannounceTracker_, onForceReannounceAllTrackers_);
}

void MainWindow::appendLog(const QString& msg) {
  LOG_INFO(QStringLiteral("[UI] %1").arg(msg));
  if (logView_ != nullptr) {
    const auto ts = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    logView_->append(QStringLiteral("[%1] %2").arg(ts, msg));
  }
}

QString MainWindow::selectedSavePath() const {
  const QString p = pfd::core::ConfigService::loadAppSettings().default_download_dir;
  return p.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) : p;
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  if (taskTable_ != nullptr && event != nullptr && event->type() == QEvent::MouseButtonPress) {
    const auto isInsideTaskTable = [this](const QObject* o) {
      for (const QObject* cur = o; cur != nullptr; cur = cur->parent()) {
        if (cur == taskTable_ || cur == taskTable_->viewport()) {
          return true;
        }
      }
      return false;
    };

    if (isInsideTaskTable(watched)) {
      if (watched == taskTable_->viewport()) {
        const auto* me = static_cast<QMouseEvent*>(event);
        const QModelIndex idx = taskTable_->indexAt(me->pos());
        if (!idx.isValid()) {
          if (transferPage_ != nullptr) {
            transferPage_->enterMemoryModeFromCurrentSelection();
          }
        }
      }
    }
  }
  return QMainWindow::eventFilter(watched, event);
}

}  // namespace pfd::ui
