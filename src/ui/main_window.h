#ifndef PFD_UI_MAIN_WINDOW_H
#define PFD_UI_MAIN_WINDOW_H

#include <QtCore/QStringList>
#include <QtWidgets/QMainWindow>

#include <cstdint>
#include <functional>
#include <vector>

class QTimer;

#include "core/task_snapshot.h"
#include "ui/add_torrent_dialog.h"
#include "ui/pages/transfer_page.h"
#include "ui/rss/rss_module_page.h"
#include "ui/search_tab.h"
#include "ui/widgets/bottom_status_bar.h"

class QLineEdit;
class QPushButton;
class QComboBox;
class QCheckBox;
class QTableWidget;
class QTextEdit;
class QLabel;
class QAction;
class QTabWidget;

namespace pfd::ui {

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  struct CopyPayload {
    QString name;
    QString infoHashV1;
    QString infoHashV2;
    QString magnet;
    QString torrentId;
    QString comment;
  };

  struct AddTorrentFileRequest {
    QString torrentFilePath;
    AddTorrentDialog::Result ui;  // 文件选择页结果
  };

  explicit MainWindow(QWidget* parent = nullptr);

  void setOnAddMagnet(std::function<void(const QString&, const QString&)> onAddMagnet);
  void setOnAddTorrentFile(std::function<void(const QString&, const QString&)> onAddTorrentFile);
  void setOnAddTorrentFileRequest(
      std::function<void(const AddTorrentFileRequest&)> onAddTorrentFileRequest);
  void setOnPauseTask(std::function<void(const pfd::base::TaskId&)> onPauseTask);
  void setOnResumeTask(std::function<void(const pfd::base::TaskId&)> onResumeTask);
  void setOnRemoveTask(std::function<void(const pfd::base::TaskId&, bool)> onRemoveTask);
  void setOnMoveTask(std::function<void(const pfd::base::TaskId&, const QString&)> onMoveTask);
  void setOnRenameTask(std::function<void(const pfd::base::TaskId&, const QString&)> onRenameTask);
  void setOnEditTrackers(
      std::function<void(const pfd::base::TaskId&, const QStringList&)> onEditTrackers);
  void
  setOnQueryTaskTrackers(std::function<QStringList(const pfd::base::TaskId&)> onQueryTaskTrackers);
  void setOnCategoryChanged(
      std::function<void(const pfd::base::TaskId&, const QString&)> onCategoryChanged);
  void
  setOnTagsChanged(std::function<void(const pfd::base::TaskId&, const QStringList&)> onTagsChanged);
  void setOnDefaultTrackersChanged(
      std::function<void(bool, const QStringList&)> onDefaultTrackersChanged);
  void setOnQueryDefaultTrackers(std::function<QStringList()> onQueryDefaultTrackers);
  void setOnQueryDefaultTrackerEnabled(std::function<bool()> onQueryDefaultTrackerEnabled);
  void setOnSeedPolicyChanged(std::function<void(bool, double)> onSeedPolicyChanged);
  void setOnSetTaskConnectionsLimit(
      std::function<void(const pfd::base::TaskId&, int)> onSetTaskConnectionsLimit);
  void setOnForceStartTask(std::function<void(const pfd::base::TaskId&)> onForceStartTask);
  void setOnForceRecheckTask(std::function<void(const pfd::base::TaskId&)> onForceRecheckTask);
  void
  setOnForceReannounceTask(std::function<void(const pfd::base::TaskId&)> onForceReannounceTask);
  void setOnSetSequentialDownload(
      std::function<void(const pfd::base::TaskId&, bool)> onSetSequentialDownload);
  void
  setOnSetAutoManagedTask(std::function<void(const pfd::base::TaskId&, bool)> onSetAutoManagedTask);
  void setOnFirstLastPiecePriority(
      std::function<void(const pfd::base::TaskId&, bool)> onFirstLastPiecePriority);
  void setOnPreviewFile(std::function<void(const pfd::base::TaskId&)> onPreviewFile);
  void setOnQueuePositionUp(std::function<void(const pfd::base::TaskId&)> cb);
  void setOnQueuePositionDown(std::function<void(const pfd::base::TaskId&)> cb);
  void setOnQueuePositionTop(std::function<void(const pfd::base::TaskId&)> cb);
  void setOnQueuePositionBottom(std::function<void(const pfd::base::TaskId&)> cb);
  void
  setOnQueryCopyPayload(std::function<CopyPayload(const pfd::base::TaskId&)> onQueryCopyPayload);
  void setOnQueryTaskFiles(std::function<std::vector<pfd::lt::SessionWorker::FileEntrySnapshot>(
                               const pfd::base::TaskId&)>
                               cb);
  void
  setOnSetTaskFilePriority(std::function<void(const pfd::base::TaskId&, const std::vector<int>&,
                                              pfd::lt::SessionWorker::FilePriorityLevel)>
                               cb);
  void setOnRenameTaskFileOrFolder(
      std::function<void(const pfd::base::TaskId&, const QString&, const QString&)> cb);
  void setOnQueryTaskTrackerSnapshot(
      std::function<pfd::lt::SessionWorker::TaskTrackerSnapshot(const pfd::base::TaskId&)> cb);
  void setOnAddTaskTracker(std::function<void(const pfd::base::TaskId&, const QString&)> cb);
  void setOnEditTaskTracker(
      std::function<void(const pfd::base::TaskId&, const QString&, const QString&)> cb);
  void setOnRemoveTaskTracker(std::function<void(const pfd::base::TaskId&, const QString&)> cb);
  void
  setOnForceReannounceTracker(std::function<void(const pfd::base::TaskId&, const QString&)> cb);
  void setOnForceReannounceAllTrackers(std::function<void(const pfd::base::TaskId&)> cb);
  void setOnQueryTaskPeers(
      std::function<std::vector<pfd::lt::SessionWorker::PeerSnapshot>(const pfd::base::TaskId&)>
          cb);
  void setOnQueryTaskWebSeeds(
      std::function<std::vector<pfd::lt::SessionWorker::WebSeedSnapshot>(const pfd::base::TaskId&)>
          cb);
  void setSearchDataSources(SearchTab::QuerySnapshotsFn snapshotsFn,
                            SearchTab::QueryRssItemsFn rssItemsFn);
  void setRssService(pfd::core::rss::RssService* service);
  /// 条目流：日志、默认下载根目录（打开文件夹兜底）、任务列表快照（按磁力反查保存路径）。
  void setRssItemsUiHelpers(std::function<void(const QString&)> appendItemsLog,
                            std::function<QString()> defaultSaveRoot,
                            std::function<std::vector<pfd::core::TaskSnapshot>()> taskSnapshots);
  void refreshRssDataViews();
  /// 切换主界面标签到「传输」（RSS 等添加任务后便于查看列表）。
  void showTransferTab();
  void appendLog(const QString& msg);
  /// 重复添加同一 info-hash 时由 AppController 调用：信息框提示，并写入日志区。
  void notifyTaskAlreadyInList(const QString& displayName = {});
  void refreshTasks(const std::vector<pfd::core::TaskSnapshot>& snapshots);
  void updateBottomStatus(int dhtNodes, qint64 downloadRate, qint64 uploadRate);

Q_SIGNALS:
  void settingsChanged();
  void rssSettingsChanged();

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void closeEvent(QCloseEvent* event) override;

private:
  using TaskFilter = TransferPage::TaskFilter;
  using SortKey = TransferPage::SortKey;
  using SortOrder = TransferPage::SortOrder;
  [[nodiscard]] pfd::base::TaskId selectedTaskId() const;
  [[nodiscard]] QString selectedSavePath() const;
  [[nodiscard]] TaskFilter currentFilter() const;
  [[nodiscard]] SortKey currentSortKey() const;
  [[nodiscard]] SortOrder currentSortOrder() const;
  void setupMenuBar();
  void applyTheme();
  void buildLayout();
  void bindSignals();
  void loadSettingsToUi();
  void loadUiState();
  void saveUiState() const;
  void openPreferences();
  void openLogCenter();
  void openTorrentFileFromMenu();
  void openTorrentLinksFromMenu();
  void showTaskContextMenu(const QPoint& pos);
  void syncContentHandlers();
  void syncTrackerHandlers();
  void refreshRssItemsTaskProgress();

  TransferPage* transferPage_{nullptr};
  rss::RssModulePage* rssModulePage_{nullptr};
  SearchTab* searchTab_{nullptr};
  QTabWidget* tabs_{nullptr};
  QTableWidget* taskTable_{nullptr};
  QTextEdit* logView_{nullptr};
  BottomStatusBar* bottomStatus_{nullptr};
  QTimer* rssRefreshDebounce_{nullptr};
  bool forceQuit_{false};
  qint64 lastRssProgressRefreshMs_{0};
  QAction* preferencesAction_{nullptr};
  QAction* themeAction_{nullptr};
  QAction* logCenterAction_{nullptr};
  QAction* helpAction_{nullptr};
  QAction* aboutAction_{nullptr};
  QAction* downloadSettingsAction_{nullptr};
  QAction* showLogAction_{nullptr};
  QAction* refreshListAction_{nullptr};
  QAction* openTorrentFileAction_{nullptr};
  QAction* openTorrentLinksAction_{nullptr};
  QAction* exitAction_{nullptr};
  std::vector<pfd::core::TaskSnapshot> snapshots_;
  std::vector<pfd::core::TaskSnapshot> displayedSnapshots_;
  std::function<void(const QString&, const QString&)> onAddMagnet_;
  std::function<void(const QString&, const QString&)> onAddTorrentFile_;
  std::function<void(const AddTorrentFileRequest&)> onAddTorrentFileRequest_;
  std::function<void(const pfd::base::TaskId&)> onPauseTask_;
  std::function<void(const pfd::base::TaskId&)> onResumeTask_;
  std::function<void(const pfd::base::TaskId&, bool)> onRemoveTask_;
  std::function<void(const pfd::base::TaskId&, const QString&)> onMoveTask_;
  std::function<void(const pfd::base::TaskId&, const QString&)> onRenameTask_;
  std::function<void(const pfd::base::TaskId&, const QStringList&)> onEditTrackers_;
  std::function<QStringList(const pfd::base::TaskId&)> onQueryTaskTrackers_;
  std::function<void(const pfd::base::TaskId&, const QString&)> onCategoryChanged_;
  std::function<void(const pfd::base::TaskId&, const QStringList&)> onTagsChanged_;
  std::function<void(bool, const QStringList&)> onDefaultTrackersChanged_;
  std::function<QStringList()> onQueryDefaultTrackers_;
  std::function<bool()> onQueryDefaultTrackerEnabled_;
  std::function<void(bool, double)> onSeedPolicyChanged_;
  std::function<void(const pfd::base::TaskId&, int)> onSetTaskConnectionsLimit_;
  std::function<void(const pfd::base::TaskId&)> onForceStartTask_;
  std::function<void(const pfd::base::TaskId&)> onForceRecheckTask_;
  std::function<void(const pfd::base::TaskId&)> onForceReannounceTask_;
  std::function<void(const pfd::base::TaskId&, bool)> onSetSequentialDownload_;
  std::function<void(const pfd::base::TaskId&, bool)> onSetAutoManagedTask_;
  std::function<void(const pfd::base::TaskId&, bool)> onFirstLastPiecePriority_;
  std::function<void(const pfd::base::TaskId&)> onPreviewFile_;
  std::function<void(const pfd::base::TaskId&)> onQueuePositionUp_;
  std::function<void(const pfd::base::TaskId&)> onQueuePositionDown_;
  std::function<void(const pfd::base::TaskId&)> onQueuePositionTop_;
  std::function<void(const pfd::base::TaskId&)> onQueuePositionBottom_;
  std::function<CopyPayload(const pfd::base::TaskId&)> onQueryCopyPayload_;
  std::function<std::vector<pfd::lt::SessionWorker::FileEntrySnapshot>(const pfd::base::TaskId&)>
      onQueryTaskFiles_;
  std::function<void(const pfd::base::TaskId&, const std::vector<int>&,
                     pfd::lt::SessionWorker::FilePriorityLevel)>
      onSetTaskFilePriority_;
  std::function<void(const pfd::base::TaskId&, const QString&, const QString&)>
      onRenameTaskFileOrFolder_;
  std::function<pfd::lt::SessionWorker::TaskTrackerSnapshot(const pfd::base::TaskId&)>
      onQueryTaskTrackerSnapshot_;
  std::function<void(const pfd::base::TaskId&, const QString&)> onAddTaskTracker_;
  std::function<void(const pfd::base::TaskId&, const QString&, const QString&)> onEditTaskTracker_;
  std::function<void(const pfd::base::TaskId&, const QString&)> onRemoveTaskTracker_;
  std::function<void(const pfd::base::TaskId&, const QString&)> onForceReannounceTracker_;
  std::function<void(const pfd::base::TaskId&)> onForceReannounceAllTrackers_;
  std::function<std::vector<pfd::lt::SessionWorker::PeerSnapshot>(const pfd::base::TaskId&)>
      onQueryTaskPeers_;
  std::function<std::vector<pfd::lt::SessionWorker::WebSeedSnapshot>(const pfd::base::TaskId&)>
      onQueryTaskWebSeeds_;
  pfd::core::rss::RssService* rssService_{nullptr};
};

}  // namespace pfd::ui

#endif  // PFD_UI_MAIN_WINDOW_H
