#ifndef PFD_APP_CONTROLLER_H
#define PFD_APP_CONTROLLER_H

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/app_settings.h"

namespace pfd::core {
class BuiltinHttpTracker;
}

#include "app/event_ingest_orchestrator.h"
#include "app/exit_coordinator.h"
#include "app/refresh_scheduler.h"
#include "app/rss_download_orchestrator.h"
#include "app/rss_download_pipeline.h"
#include "app/task_batch_use_case.h"
#include "app/task_control_command_use_case.h"
#include "app/task_detail_query_use_case.h"
#include "app/task_meta_command_use_case.h"
#include "app/task_persistence_coordinator.h"
#include "core/rss/rss_service.h"
#include "core/save_path_policy.h"
#include "core/task_pipeline_service.h"
#include "lt/session_worker.h"
#include "ui/main_window.h"
#include "ui/system_tray.h"

class QTimer;
class QFileSystemWatcher;

class QApplication;

namespace pfd::app {

inline constexpr int kTasksJsonVersion = 1;

class AppController {
public:
  AppController(QApplication* app, pfd::ui::MainWindow* window,
                pfd::core::TaskPipelineService* pipeline, pfd::lt::SessionWorker* worker);
  ~AppController();

  void initialize();
  void submitArgvMagnet(const QString& magnet);

private:
  void updateRssTimerInterval();
  void loadSettings();
  void saveSettings() const;
  void bindUiCallbacks();
  void bindUiAddCallbacks();
  void bindUiTaskControlCallbacks();
  void bindUiTaskDetailCallbacks();
  void bindUiTrackerCallbacks();
  void bindUiTaskMetaCallbacks();
  void bindUiMiscCallbacks();
  void bindWorkerCallbacks();
  void scheduleTimedAction();
  void executeTimedAction();
  void maybeRunPostDownloadAction(const std::vector<pfd::core::TaskSnapshot>& snapshots);
  void executePostDownloadAction(const QString& action);
  void applySeedingPolicy(const std::vector<pfd::core::TaskSnapshot>& snapshots);
  void applyTaskMetaUpdate(const pfd::base::TaskId& taskId, const QString& name,
                           const QString& savePath, const QString& category,
                           const QString& tagsCsv);
  void applyRuntimeSettingsFromConfig(const pfd::core::AppSettings* app_settings = nullptr);
  [[nodiscard]] pfd::core::rss::RssFetcher::HttpProxyConfig httpProxyConfigForRss() const;
  void syncIpFilterFileWatcher();
  void applyBuiltinHttpTrackerFromSettings(const pfd::core::AppSettings& s);
  void loadPersistedTasks();
  void savePersistedTasks() const;
  void saveResumeData() const;
  void schedulePersistedTasksAutoSave();
  void initializeRss();
  /// 对诊断为可自动下载的条目调用
  /// `RssService::downloadItem`（与条目流「下载选中」相同路径），受并发与单次批量上限约束。
  void pumpRssDeferredAutoDownloads(pfd::core::rss::RssAutoDownloadScope scope =
                                        pfd::core::rss::RssAutoDownloadScope::kAllEligible,
                                    bool skip_diagnosis = false);
  void settleRssDownloadIfNeeded(const pfd::core::rss::RssDownloadSettlement& s, bool ok,
                                 const QString& resolved_save_override = {});
  void scheduleRssPostSettleFollowUp();
  void upsertKnownTaskMagnet(const pfd::base::TaskId& taskId, const QString& magnet);
  void injectTransitionalStatus(const pfd::base::TaskId& taskId, pfd::base::TaskStatus status);
  [[nodiscard]] bool restoreStartPaused() const;
  void logInfo(const QString& msg) const;
  void logError(const QString& msg) const;

  [[nodiscard]] static pfd::ui::AddTorrentDialog::MagnetInput
  magnetInputFromMetadata(const pfd::lt::SessionWorker::MagnetMetadata& meta);
  void submitPreparedMagnetAdd(const pfd::lt::SessionWorker::MagnetMetadata& meta,
                               const pfd::ui::AddTorrentDialog::Result& ui);
  void submitTorrentFileFromDialog(const QString& torrentFilePath,
                                   const pfd::ui::AddTorrentDialog::Result& ui);
  [[nodiscard]] bool presentMagnetAddDialogViaExportedTorrent(
      const pfd::lt::SessionWorker::MagnetMetadata& meta, const QString& defaultSavePath,
      const pfd::core::rss::RssDownloadSettlement& rssSettlement);
  [[nodiscard]] bool
  showMagnetAddDialogAndSubmit(const pfd::lt::SessionWorker::MagnetMetadata& meta,
                               const QString& defaultSavePath,
                               const pfd::core::rss::RssDownloadSettlement& rssSettlement);
  void submitMagnetDialogResult(const pfd::lt::SessionWorker::MagnetMetadata& meta,
                                const pfd::ui::AddTorrentDialog::Result& ui,
                                const pfd::core::rss::RssDownloadSettlement& rssSettlement);

  void enqueueMagnet(const QString& uri, const QString& savePath,
                     const pfd::core::rss::RssDownloadSettlement& rssSettlement = {},
                     bool skipInteractiveAdd = false, const QString& category = {},
                     const QString& tagsCsv = {});
  /// 与「打开 Torrent 文件」相同：拉取元数据后弹出 AddTorrentDialog 供选择文件与选项。
  void presentInteractiveMagnetAdd(const QString& uri, const QString& savePathHint = {});
  void startOneMagnetOnUi(pfd::app::RssDownloadPipeline::MagnetQueueItem item);
  void enqueueRssTorrentUrl(const QString& url, const QString& savePath,
                            const QString& referer = {},
                            const pfd::core::rss::RssDownloadSettlement& rssSettlement = {});
  void startOneRssTorrentUrlOnUi(pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem item);

  QApplication* app_{nullptr};
  pfd::ui::MainWindow* window_{nullptr};
  pfd::core::TaskPipelineService* pipeline_{nullptr};
  pfd::lt::SessionWorker* worker_{nullptr};

  std::unordered_set<QString> autoStoppedBySeedRatio_;
  std::unordered_set<QString> autoStoppedBySeedTime_;
  std::unordered_map<QString, QStringList> taskTrackers_;
  QStringList defaultTrackers_;
  bool autoApplyDefaultTrackers_{false};
  bool seedUnlimited_{false};
  double autoStopSeedRatio_{1.0};
  int seedMaxMinutes_{0};
  int perTorrentConnectionsLimit_{0};
  pfd::core::SavePathPolicy savePathPolicy_;

  int magnetMaxInFlight_{8};  // 同时最多弹出 5-10 个，默认取 8

  std::mutex magnetTasksMu_;
  std::vector<std::future<void>> magnetTasks_;
  std::atomic<bool> shuttingDown_{false};

  std::atomic<bool> statusQueryRunning_{false};
  std::future<void> statusTask_;

  bool bottomStatusEnabled_{true};
  int bottomStatusRefreshMs_{1000};
  int taskAutoSaveMs_{5000};
  bool restoreStartPaused_{true};
  pfd::core::AppSettings cachedAppSettings_{};
  std::unordered_map<QString, QString> knownTaskMagnets_;
  /// 与上次成功落盘的 tasks.json 内容一致则跳过写入，减少磁盘抖动。
  mutable QByteArray lastPersistedTasksJson_{};

  std::unique_ptr<pfd::core::BuiltinHttpTracker> builtinHttpTracker_;
  std::unique_ptr<pfd::core::rss::RssService> rssService_;
  QTimer* rssTimer_{nullptr};
  QTimer* timedActionTimer_{nullptr};

  pfd::ui::SystemTray* systemTray_{nullptr};
  std::unordered_set<QString> notifiedFinished_;
  bool postDownloadActionTriggered_{false};

  std::unique_ptr<pfd::app::RefreshScheduler> uiRefreshScheduler_;
  /// 合并 RSS 结算后的 saveState / 视图刷新 / 续派，避免批量自动下载时 UI 线程被连续重入拖死。
  std::unique_ptr<pfd::app::RefreshScheduler> rssPostSettleScheduler_;
  std::unique_ptr<pfd::app::TaskBatchUseCase> taskBatchUseCase_;
  std::unique_ptr<pfd::app::TaskControlCommandUseCase> taskControlCommandUseCase_;
  std::unique_ptr<pfd::app::TaskDetailQueryUseCase> taskDetailQueryUseCase_;
  std::unique_ptr<pfd::app::TaskMetaCommandUseCase> taskMetaCommandUseCase_;
  std::unique_ptr<pfd::app::RssDownloadOrchestrator> rssDownloadOrchestrator_;
  std::unique_ptr<pfd::app::TaskPersistenceCoordinator> taskPersistenceCoordinator_;
  pfd::app::ExitCoordinator exitCoordinator_;
  QFileSystemWatcher* ipFilterFileWatcher_{nullptr};

  std::unique_ptr<pfd::app::EventIngestOrchestrator> eventIngestOrchestrator_;
};

}  // namespace pfd::app

#endif  // PFD_APP_CONTROLLER_H
