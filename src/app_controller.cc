#include "app_controller.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QPointer>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QtWidgets/QApplication>
#include <QtWidgets/QSystemTrayIcon>

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/magnet_uri.hpp>

#include <algorithm>
#include <future>
#include <thread>

#include "app/task_query_mapper.h"
#include "base/input_sanitizer.h"
#include "base/io.h"
#include "base/paths.h"
#include "core/builtin_http_tracker.h"
#include "core/config_service.h"
#include "core/logger.h"
#include "core/rss/rss_fetcher.h"
#include "core/save_path_policy.h"
#include "core/task_add_builder.h"
#include "core/task_event.h"
#include "core/task_snapshot.h"
#include "lt/session_ids.h"

namespace pfd::app {

AppController::AppController(QApplication* app, pfd::ui::MainWindow* window,
                             pfd::core::TaskPipelineService* pipeline,
                             pfd::lt::SessionWorker* worker)
    : app_(app), window_(window), pipeline_(pipeline), worker_(worker) {}

AppController::~AppController() {
  shuttingDown_.store(true);
  builtinHttpTracker_.reset();
  constexpr auto kWaitBudgetPerTask = std::chrono::milliseconds(120);
  std::vector<std::future<void>> tasks;
  {
    std::lock_guard<std::mutex> lk(magnetTasksMu_);
    tasks.swap(magnetTasks_);
  }
  for (auto& f : tasks) {
    waitFutureAtMost(f, kWaitBudgetPerTask);
  }
  waitFutureAtMost(statusTask_, kWaitBudgetPerTask);
  saveSettings();
}

void AppController::initialize() {
  loadSettings();

  taskBatchUseCase_ = std::make_unique<pfd::app::TaskBatchUseCase>(
      worker_, [this]() { return pipeline_->snapshots(); });
  taskControlCommandUseCase_ = std::make_unique<pfd::app::TaskControlCommandUseCase>(
      [this](const pfd::base::TaskId& taskId) { worker_->pauseTask(taskId); },
      [this](const pfd::base::TaskId& taskId) { worker_->resumeTask(taskId); },
      [this](const pfd::base::TaskId& taskId, bool removeFiles) {
        worker_->removeTask(taskId, removeFiles);
      },
      [this](const pfd::base::TaskId& taskId, int limit) {
        worker_->setTaskConnectionsLimit(taskId, limit);
      },
      [this](const pfd::base::TaskId& taskId) { worker_->forceStartTask(taskId); },
      [this](const pfd::base::TaskId& taskId) { worker_->forceRecheckTask(taskId); },
      [this](const pfd::base::TaskId& taskId) { worker_->forceReannounceTask(taskId); },
      [this](const pfd::base::TaskId& taskId, bool enabled) {
        worker_->setSequentialDownload(taskId, enabled);
      },
      [this](const pfd::base::TaskId& taskId, bool enabled) {
        worker_->setAutoManagedTask(taskId, enabled);
      },
      [this](const QString& msg) { logInfo(msg); });
  taskDetailQueryUseCase_ = std::make_unique<pfd::app::TaskDetailQueryUseCase>(pipeline_, worker_);
  taskMetaCommandUseCase_ = std::make_unique<pfd::app::TaskMetaCommandUseCase>(
      [this](const pfd::core::TaskEvent& ev) { pipeline_->consume(ev); },
      [this](const QString& msg) { logError(msg); });
  eventIngestOrchestrator_ = std::make_unique<pfd::app::EventIngestOrchestrator>(pipeline_);
  rssDownloadPipeline_ =
      std::make_unique<pfd::app::RssDownloadPipeline>(static_cast<QObject*>(app_));
  uiRefreshScheduler_ =
      std::make_unique<pfd::app::RefreshScheduler>(static_cast<QObject*>(app_), 100, [this]() {
        const auto snaps = pipeline_->snapshots();
        window_->refreshTasks(snaps);
        applySeedingPolicy(snaps);
      });
  taskPersistenceCoordinator_ = std::make_unique<pfd::app::TaskPersistenceCoordinator>(
      static_cast<QObject*>(app_), [this]() { savePersistedTasks(); },
      [this]() { saveResumeData(); });
  loadPersistedTasks();

  bindUiCallbacks();
  bindWorkerCallbacks();
  applyRuntimeSettingsFromConfig(&cachedAppSettings_);
  schedulePersistedTasksAutoSave();
  initializeRss();
  if (window_) {
    window_->setRssItemsUiHelpers(
        [this](const QString& m) {
          if (window_)
            window_->appendLog(m);
        },
        [this]() { return savePathPolicy_.resolve(QString()); },
        [this]() {
          return pipeline_ ? pipeline_->snapshots() : std::vector<pfd::core::TaskSnapshot>{};
        });
  }

  QObject::connect(window_, &pfd::ui::MainWindow::settingsChanged, static_cast<QObject*>(app_),
                   [this]() {
                     loadSettings();
                     applyRuntimeSettingsFromConfig(&cachedAppSettings_);
                     scheduleTimedAction();
                     updateRssTimerInterval();
                     logInfo(QStringLiteral("Runtime settings updated from preferences."));
                   });
  logInfo(
      QStringLiteral("Session started. Use 菜单：文件 -> 打开 Torrent 文件 / 打开 Torrent 链接。"));

  systemTray_ = new pfd::ui::SystemTray(static_cast<QObject*>(app_));
  systemTray_->show();
  QApplication::setQuitOnLastWindowClosed(false);
  if (cachedAppSettings_.start_minimized && QSystemTrayIcon::isSystemTrayAvailable()) {
    window_->hide();
  }
  scheduleTimedAction();
  QObject::connect(systemTray_, &pfd::ui::SystemTray::showWindowRequested,
                   static_cast<QObject*>(app_), [this]() {
                     if (window_) {
                       window_->show();
                       window_->raise();
                       window_->activateWindow();
                     }
                   });
  QObject::connect(systemTray_, &pfd::ui::SystemTray::quitRequested, static_cast<QObject*>(app_),
                   [this]() { app_->quit(); });
  QObject::connect(systemTray_, &pfd::ui::SystemTray::pauseAllRequested,
                   static_cast<QObject*>(app_),
                   [this]() { taskBatchUseCase_->pauseAllActiveTasks(); });
  QObject::connect(systemTray_, &pfd::ui::SystemTray::resumeAllRequested,
                   static_cast<QObject*>(app_),
                   [this]() { taskBatchUseCase_->resumeAllPausedTasks(); });

  QObject::connect(app_, &QApplication::aboutToQuit, static_cast<QObject*>(app_), [this]() {
    shuttingDown_.store(true);
    if (taskPersistenceCoordinator_) {
      // 快速退出：优先保存任务元数据，跳过耗时 resume 全量写盘。
      taskPersistenceCoordinator_->saveTasksNow();
    }
    if (rssService_)
      rssService_->saveState();
  });

  if (!bottomStatusEnabled_) {
    return;
  }

  auto* timer = new QTimer(static_cast<QObject*>(app_));
  timer->setInterval(bottomStatusRefreshMs_);
  QObject::connect(timer, &QTimer::timeout, static_cast<QObject*>(app_), [this]() {
    if (shuttingDown_.load()) {
      return;
    }
    bool expected = false;
    if (!statusQueryRunning_.compare_exchange_strong(expected, true)) {
      return;
    }
    QPointer<pfd::ui::MainWindow> w(window_);
    auto* workerPtr = worker_;
    statusTask_ = std::async(std::launch::async, [this, w, workerPtr]() {
      const auto st = workerPtr->querySessionStats();
      if (shuttingDown_.load() || w.isNull()) {
        statusQueryRunning_.store(false);
        return;
      }
      QObject* target = static_cast<QObject*>(w.data());
      QMetaObject::invokeMethod(
          target,
          [this, w, st]() {
            if (!w.isNull()) {
              w->updateBottomStatus(st.dht_nodes, st.download_rate, st.upload_rate);
            }
            statusQueryRunning_.store(false);
          },
          Qt::QueuedConnection);
    });
  });
  timer->start();
}

void AppController::schedulePersistedTasksAutoSave() {
  taskPersistenceCoordinator_->setAutoSaveIntervalMs(taskAutoSaveMs_);
  taskPersistenceCoordinator_->startAutoSave();
}

void AppController::waitFutureAtMost(std::future<void>& fut,
                                     std::chrono::milliseconds timeout) const {
  if (!fut.valid()) {
    return;
  }
  if (fut.wait_for(timeout) == std::future_status::ready) {
    fut.wait();
  }
}

void AppController::scheduleTimedAction() {
  if (timedActionTimer_ == nullptr) {
    timedActionTimer_ = new QTimer(static_cast<QObject*>(app_));
    timedActionTimer_->setSingleShot(true);
    QObject::connect(timedActionTimer_, &QTimer::timeout, static_cast<QObject*>(app_),
                     [this]() { executeTimedAction(); });
  }
  timedActionTimer_->stop();

  const QString action = cachedAppSettings_.timed_action.trimmed().toLower();
  const int delayMinutes = cachedAppSettings_.timed_action_delay_minutes;
  if (action == QStringLiteral("none") || delayMinutes <= 0) {
    logInfo(QStringLiteral("Timed action disabled."));
    return;
  }

  timedActionTimer_->start(delayMinutes * 60 * 1000);
  logInfo(QStringLiteral("Timed action scheduled. action=%1 delay_minutes=%2")
              .arg(action)
              .arg(delayMinutes));
}

void AppController::executeTimedAction() {
  const QString action = cachedAppSettings_.timed_action.trimmed().toLower();
  if (action == QStringLiteral("quit_app")) {
    logInfo(QStringLiteral("Timed action triggered: quit application."));
    app_->quit();
    return;
  }
  if (action == QStringLiteral("shutdown")) {
    QString command;
    QStringList args;
#ifdef _WIN32
    command = QStringLiteral("shutdown");
    args = QStringList{QStringLiteral("/s"), QStringLiteral("/t"), QStringLiteral("0")};
#else
    command = QStringLiteral("/bin/sh");
    args = QStringList{QStringLiteral("-c"), QStringLiteral("shutdown -h now")};
#endif
    const bool launched = QProcess::startDetached(command, args);
    if (launched) {
      logInfo(QStringLiteral("Timed action triggered: shutdown command issued."));
      app_->quit();
    } else {
      logError(QStringLiteral("Timed action failed: unable to launch shutdown command."));
    }
    return;
  }
  logInfo(QStringLiteral("Timed action skipped: unknown action=%1").arg(action));
}

void AppController::maybeRunPostDownloadAction(
    const std::vector<pfd::core::TaskSnapshot>& snapshots) {
  const bool timedActionEnabled =
      cachedAppSettings_.timed_action.trimmed().toLower() != QStringLiteral("none") &&
      cachedAppSettings_.timed_action_delay_minutes > 0;
  if (timedActionEnabled) {
    postDownloadActionTriggered_ = false;
    return;
  }
  if (snapshots.empty()) {
    postDownloadActionTriggered_ = false;
    return;
  }

  bool allFinished = true;
  for (const auto& s : snapshots) {
    if (!pfd::base::isFinished(s.status)) {
      allFinished = false;
      break;
    }
  }
  if (!allFinished) {
    postDownloadActionTriggered_ = false;
    return;
  }
  if (postDownloadActionTriggered_) {
    return;
  }
  postDownloadActionTriggered_ = true;
  executePostDownloadAction(cachedAppSettings_.download_complete_action.trimmed().toLower());
}

void AppController::executePostDownloadAction(const QString& action) {
  if (action == QStringLiteral("none") || action.isEmpty()) {
    return;
  }
  if (action == QStringLiteral("quit_app")) {
    logInfo(QStringLiteral("Post-download action triggered: quit application."));
    app_->quit();
    return;
  }

  QString command = QStringLiteral("/bin/sh");
  QStringList args;
#ifdef _WIN32
  if (action == QStringLiteral("poweroff")) {
    command = QStringLiteral("shutdown");
    args = QStringList{QStringLiteral("/s"), QStringLiteral("/t"), QStringLiteral("0")};
  }
#else
  if (action == QStringLiteral("suspend")) {
    args = QStringList{QStringLiteral("-c"), QStringLiteral("systemctl suspend")};
  } else if (action == QStringLiteral("hibernate")) {
    args = QStringList{QStringLiteral("-c"), QStringLiteral("systemctl hibernate")};
  } else if (action == QStringLiteral("poweroff")) {
    args = QStringList{QStringLiteral("-c"), QStringLiteral("systemctl poweroff")};
  }
#endif

  if (args.isEmpty()) {
    logInfo(QStringLiteral("Post-download action skipped: unknown action=%1").arg(action));
    return;
  }
  if (QProcess::startDetached(command, args)) {
    logInfo(QStringLiteral("Post-download action triggered: %1").arg(action));
    if (action == QStringLiteral("poweroff")) {
      app_->quit();
    }
    return;
  }
  logError(QStringLiteral("Post-download action failed: unable to launch action=%1").arg(action));
}

void AppController::applyRuntimeSettingsFromConfig(const pfd::core::AppSettings* app_settings) {
  const auto s =
      app_settings != nullptr ? *app_settings : pfd::core::ConfigService::loadAppSettings();
  cachedAppSettings_ = s;
  postDownloadActionTriggered_ = false;
  savePathPolicy_.setDefaultDownloadDir(s.default_download_dir);
  seedUnlimited_ = s.seed_unlimited;
  autoStopSeedRatio_ = s.seed_target_ratio;
  seedMaxMinutes_ = s.seed_max_minutes;
  perTorrentConnectionsLimit_ = s.per_torrent_connections_limit;
  autoStoppedBySeedRatio_.clear();
  autoStoppedBySeedTime_.clear();
  worker_->applyRuntimeSettings(
      s.download_rate_limit_kib, s.upload_rate_limit_kib, s.connections_limit, s.upload_slots_limit,
      s.per_torrent_upload_slots_limit, s.listen_port, s.listen_port_forwarding, s.active_downloads,
      s.active_seeds, s.active_limit, s.max_active_checking, s.auto_manage_prefer_seeds,
      s.dont_count_slow_torrents, s.slow_torrent_dl_rate_threshold,
      s.slow_torrent_ul_rate_threshold, s.slow_torrent_inactive_timer, s.enable_dht, s.enable_upnp,
      s.enable_natpmp, s.enable_lsd, s.proxy_enabled, s.proxy_type, s.proxy_host, s.proxy_port,
      s.proxy_username, s.proxy_password, s.encryption_mode, s.ip_filter_enabled, s.ip_filter_path,
      s.monitor_port);
  worker_->setDefaultPerTorrentConnectionsLimit(perTorrentConnectionsLimit_);
  applyBuiltinHttpTrackerFromSettings(s);
  if (rssService_ != nullptr) {
    rssService_->setRequestHeaders(pfd::core::rss::RssFetcher::RequestHeaders{
        s.http_user_agent, s.http_accept_language, s.http_cookie_header, s.http_cookie_rules});
  }
  if (window_ != nullptr) {
    window_->setSearchRequestHeaders(pfd::ui::SearchTab::RequestHeaders{
        s.http_user_agent, s.http_accept_language, s.http_cookie_header});
  }
}

void AppController::applyBuiltinHttpTrackerFromSettings(const pfd::core::AppSettings& s) {
  if (!builtinHttpTracker_) {
    builtinHttpTracker_ =
        std::make_unique<pfd::core::BuiltinHttpTracker>(static_cast<QObject*>(app_));
  }
  const quint16 port = static_cast<quint16>(std::clamp(s.builtin_tracker_port, 0, 65535));
  builtinHttpTracker_->apply(s.builtin_tracker_enabled, port);
  if (s.builtin_tracker_enabled && s.builtin_tracker_port_forwarding) {
    logInfo(QStringLiteral(
        "builtin tracker: port forwarding from preferences is not applied in P0 (listen "
        "127.0.0.1 only)."));
  }
}

void AppController::submitArgvMagnet(const QString& magnet) {
  const auto trimmed = magnet.trimmed();
  if (trimmed.isEmpty()) {
    return;
  }
  const auto valErr = pfd::base::validateMagnetUri(trimmed);
  if (valErr.hasError()) {
    logError(QStringLiteral("Argv magnet rejected: %1").arg(valErr.message()));
    return;
  }
  libtorrent::error_code ec;
  const auto atp = libtorrent::parse_magnet_uri(trimmed.toStdString(), ec);
  if (!ec) {
    upsertKnownTaskMagnet(pfd::lt::session_ids::taskIdFromInfoHashes(atp.info_hashes), trimmed);
  }
  logInfo(QStringLiteral("Magnet URI from argv detected."));
  enqueueMagnet(trimmed, QString());
}

void AppController::updateRssTimerInterval() {
  if (!rssTimer_ || !rssService_)
    return;
  const int ms = rssService_->settings().refresh_interval_minutes * 60 * 1000;
  rssTimer_->setInterval(ms);
  logInfo(QStringLiteral("RSS timer interval updated to %1 ms").arg(ms));
}

void AppController::enqueueMagnet(const QString& uri, const QString& savePath,
                                  const pfd::core::rss::RssDownloadSettlement& rssSettlement,
                                  bool skipInteractiveAdd, const QString& category,
                                  const QString& tagsCsv) {
  rssDownloadPipeline_->setMagnetMaxInFlight(magnetMaxInFlight_);
  rssDownloadPipeline_->enqueueMagnet(
      pfd::app::RssDownloadPipeline::MagnetQueueItem{uri, savePath, rssSettlement,
                                                     skipInteractiveAdd, category, tagsCsv},
      [this](pfd::app::RssDownloadPipeline::MagnetQueueItem item) {
        if (!shuttingDown_.load()) {
          startOneMagnetOnUi(std::move(item));
        } else {
          settleRssDownloadIfNeeded(item.rssSettlement, false);
          rssDownloadPipeline_->finishMagnet(
              [this](pfd::app::RssDownloadPipeline::MagnetQueueItem next) {
                startOneMagnetOnUi(std::move(next));
              });
        }
      });
}

void AppController::settleRssDownloadIfNeeded(const pfd::core::rss::RssDownloadSettlement& s,
                                              bool ok, const QString& resolved_save_override) {
  if (s.item_id.isEmpty() || !rssService_) {
    return;
  }
  rssService_->applyRssDownloadSettlement(s, ok, resolved_save_override);
  if (ok && window_) {
    window_->refreshRssDataViews();
  }
}

void AppController::startOneMagnetOnUi(pfd::app::RssDownloadPipeline::MagnetQueueItem item) {
  const pfd::core::rss::RssDownloadSettlement rssS = item.rssSettlement;
  if (shuttingDown_.load()) {
    settleRssDownloadIfNeeded(rssS, false);
    rssDownloadPipeline_->finishMagnet([this](pfd::app::RssDownloadPipeline::MagnetQueueItem next) {
      startOneMagnetOnUi(std::move(next));
    });
    return;
  }
  const QString trimmed = item.uri.trimmed();
  const auto magnetErr = pfd::base::validateMagnetUri(trimmed);
  if (magnetErr.hasError()) {
    settleRssDownloadIfNeeded(rssS, false);
    if (!trimmed.isEmpty()) {
      logError(QStringLiteral("Magnet URI rejected: %1").arg(magnetErr.message()));
    }
    rssDownloadPipeline_->finishMagnet([this](pfd::app::RssDownloadPipeline::MagnetQueueItem next) {
      startOneMagnetOnUi(std::move(next));
    });
    return;
  }

  const QString resolvedSavePath = savePathPolicy_.resolve(item.savePath);
  const QString tempMetaPath = QDir(resolvedSavePath).filePath(QStringLiteral(".pfd_meta"));
  QDir().mkpath(tempMetaPath);
  logInfo(QStringLiteral("Magnet metadata fetching..."));

  QPointer<pfd::ui::MainWindow> windowGuard(window_);
  auto* workerPtr = worker_;
  const bool useDefaultTrackers = autoApplyDefaultTrackers_;
  const QStringList defaultTrackersCopy = defaultTrackers_;
  const bool skipInteractiveAdd = item.skipInteractiveAdd;
  const QString magnetSavePathRaw = item.savePath;
  const QString magnetCategory = item.category;
  const QString magnetTagsCsv = item.tagsCsv;

  auto fut = std::async(std::launch::async, [this, windowGuard, workerPtr, trimmed,
                                             resolvedSavePath, tempMetaPath, useDefaultTrackers,
                                             defaultTrackersCopy, rssS, skipInteractiveAdd,
                                             magnetSavePathRaw, magnetCategory, magnetTagsCsv]() {
    const auto metaOpt = workerPtr->prepareMagnetMetadata(trimmed, tempMetaPath, 12000);
    if (shuttingDown_.load() || windowGuard.isNull()) {
      QMetaObject::invokeMethod(
          static_cast<QObject*>(app_),
          [this, rssS]() {
            settleRssDownloadIfNeeded(rssS, false);
            rssDownloadPipeline_->finishMagnet(
                [this](pfd::app::RssDownloadPipeline::MagnetQueueItem next) {
                  startOneMagnetOnUi(std::move(next));
                });
          },
          Qt::QueuedConnection);
      return;
    }
    QObject* target = static_cast<QObject*>(windowGuard.data());
    QMetaObject::invokeMethod(
        target,
        [=]() {
          // 任何结果（成功/失败/取消）都要释放 in-flight 并继续 pump
          const auto finish = [this]() {
            rssDownloadPipeline_->finishMagnet(
                [this](pfd::app::RssDownloadPipeline::MagnetQueueItem next) {
                  startOneMagnetOnUi(std::move(next));
                });
          };

          if (windowGuard.isNull()) {
            settleRssDownloadIfNeeded(rssS, false);
            finish();
            return;
          }
          if (!metaOpt.has_value()) {
            logError(QStringLiteral("获取磁力链接元数据超时或失败。"));
            settleRssDownloadIfNeeded(rssS, false);
            finish();
            return;
          }
          const auto meta = *metaOpt;

          pfd::ui::AddTorrentDialog::MagnetInput in;
          in.name = meta.name;
          in.totalBytes = meta.totalBytes;
          in.creationDate = meta.creationDate;
          in.infoHashV1 = meta.infoHashV1;
          in.infoHashV2 = meta.infoHashV2;
          in.comment = meta.comment;
          in.filePaths = meta.filePaths;
          in.fileSizes = meta.fileSizes;

          if (skipInteractiveAdd) {
            QString cat = magnetCategory.trimmed();
            if (const auto cErr = pfd::base::validateCategory(cat); cErr.hasError()) {
              logError(QStringLiteral("[rss-dl] 无效分类已忽略：%1").arg(cErr.message()));
              cat.clear();
            }
            QString tags = magnetTagsCsv.trimmed();
            if (const auto tErr = pfd::base::validateTagsCsv(tags); tErr.hasError()) {
              logError(QStringLiteral("[rss-dl] 无效标签已忽略：%1").arg(tErr.message()));
              tags.clear();
            }
            pfd::core::TaskAddInput tai;
            tai.name = meta.name;
            tai.savePath = magnetSavePathRaw;
            tai.layout = pfd::core::ContentLayout::kOriginal;
            tai.startTorrent = true;
            tai.stopCondition = 0;
            tai.sequentialDownload = false;
            tai.skipHashCheck = false;
            tai.addToTopQueue = false;
            tai.category = cat;
            tai.tagsCsv = tags;
            tai.fileWanted.assign(meta.filePaths.size(), 1);
            const auto built = pfd::core::buildTaskAdd(tai, savePathPolicy_);

            const QStringList trackers = useDefaultTrackers ? defaultTrackersCopy : QStringList{};
            pfd::lt::SessionWorker::AddTorrentOptions opts;
            opts.file_priorities = built.opts.file_priorities;
            opts.start_torrent = built.opts.start_torrent;
            opts.stop_when_ready = built.opts.stop_when_ready;
            opts.sequential_download = built.opts.sequential_download;
            opts.skip_hash_check = built.opts.skip_hash_check;
            opts.add_to_top_queue = built.opts.add_to_top_queue;
            opts.category = built.opts.category;
            opts.tags_csv = built.opts.tags_csv;

            workerPtr->finalizePreparedMagnet(meta.taskId, built.finalSavePath, trackers, opts);
            applyTaskMetaUpdate(meta.taskId, meta.name, built.finalSavePath, cat, tags);
            logInfo(QStringLiteral("[rss-dl] 已添加磁力任务：%1 → %2")
                        .arg(meta.name, built.finalSavePath));
            settleRssDownloadIfNeeded(rssS, true, built.finalSavePath);
            if (windowGuard) {
              windowGuard->showTransferTab();
            }
            finish();
            return;
          }

          const auto uiOpt = pfd::ui::AddTorrentDialog::runForMagnetMetadata(windowGuard.data(), in,
                                                                             resolvedSavePath);
          if (!uiOpt.has_value()) {
            workerPtr->cancelPreparedMagnet(meta.taskId);
            logInfo(QStringLiteral("已取消添加磁力链接：%1").arg(meta.name));
            settleRssDownloadIfNeeded(rssS, false);
            finish();
            return;
          }

          const auto& ui = *uiOpt;
          pfd::core::TaskAddInput tai;
          tai.name = ui.name;
          tai.savePath = ui.savePath;
          tai.layout = static_cast<pfd::core::ContentLayout>(static_cast<int>(ui.layout));
          tai.startTorrent = ui.startTorrent;
          tai.stopCondition = ui.stopCondition;
          tai.sequentialDownload = ui.sequentialDownload;
          tai.skipHashCheck = ui.skipHashCheck;
          tai.addToTopQueue = ui.addToTopQueue;
          tai.category = ui.category;
          tai.tagsCsv = ui.tagsCsv;
          tai.fileWanted = ui.fileWanted;
          const auto built = pfd::core::buildTaskAdd(tai, savePathPolicy_);

          const QStringList trackers = useDefaultTrackers ? defaultTrackersCopy : QStringList{};
          pfd::lt::SessionWorker::AddTorrentOptions opts;
          opts.file_priorities = built.opts.file_priorities;
          opts.start_torrent = built.opts.start_torrent;
          opts.stop_when_ready = built.opts.stop_when_ready;
          opts.sequential_download = built.opts.sequential_download;
          opts.skip_hash_check = built.opts.skip_hash_check;
          opts.add_to_top_queue = built.opts.add_to_top_queue;
          opts.category = built.opts.category;
          opts.tags_csv = built.opts.tags_csv;

          workerPtr->finalizePreparedMagnet(meta.taskId, built.finalSavePath, trackers, opts);
          applyTaskMetaUpdate(meta.taskId, ui.name, built.finalSavePath, ui.category, ui.tagsCsv);
          logInfo(QStringLiteral("Magnet submitted with selection. name=%1 savePath=%2")
                      .arg(ui.name, built.finalSavePath));
          settleRssDownloadIfNeeded(rssS, true, built.finalSavePath);
          finish();
        },
        Qt::QueuedConnection);
  });
  {
    std::lock_guard<std::mutex> lk(magnetTasksMu_);
    magnetTasks_.push_back(std::move(fut));
  }
}

void AppController::enqueueRssTorrentUrl(
    const QString& url, const QString& savePath, const QString& referer,
    const pfd::core::rss::RssDownloadSettlement& rssSettlement) {
  rssDownloadPipeline_->enqueueRssTorrentUrl(
      pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem{url.trimmed(), savePath, referer,
                                                            rssSettlement},
      [this](pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem item) {
        if (!shuttingDown_.load()) {
          startOneRssTorrentUrlOnUi(std::move(item));
        } else {
          settleRssDownloadIfNeeded(item.rssSettlement, false);
          rssDownloadPipeline_->finishRssTorrent(
              [this](pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem next) {
                startOneRssTorrentUrlOnUi(std::move(next));
              });
        }
      });
}

void AppController::startOneRssTorrentUrlOnUi(
    pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem item) {
  const QString url = item.url.trimmed();
  const QString referer = item.referer.trimmed();
  const pfd::core::rss::RssDownloadSettlement rssS = item.rssSettlement;
  if (url.isEmpty()) {
    settleRssDownloadIfNeeded(rssS, false);
    logError(QStringLiteral("RSS torrent URL is empty."));
    rssDownloadPipeline_->finishRssTorrent(
        [this](pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem next) {
          startOneRssTorrentUrlOnUi(std::move(next));
        });
    return;
  }

  if (shuttingDown_.load()) {
    settleRssDownloadIfNeeded(rssS, false);
    rssDownloadPipeline_->finishRssTorrent(
        [this](pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem next) {
          startOneRssTorrentUrlOnUi(std::move(next));
        });
    return;
  }

  const QString resolvedSavePath = savePathPolicy_.resolve(item.savePath);

  QPointer<pfd::ui::MainWindow> windowGuard(window_);
  auto* workerPtr = worker_;
  const bool useDefaultTrackers = autoApplyDefaultTrackers_;
  const QStringList defaultTrackersCopy = defaultTrackers_;

  const auto requestHeaders = pfd::core::rss::RssFetcher::RequestHeaders{
      cachedAppSettings_.http_user_agent, cachedAppSettings_.http_accept_language,
      cachedAppSettings_.http_cookie_header, cachedAppSettings_.http_cookie_rules};
  auto fut = std::async(std::launch::async, [this, windowGuard, workerPtr, url, referer,
                                             resolvedSavePath, useDefaultTrackers,
                                             defaultTrackersCopy, rssS, requestHeaders]() {
    pfd::core::rss::RssFetcher fetcher;
    fetcher.setRequestHeaders(requestHeaders);
    const auto res = fetcher.fetch(url, referer);

    QString torrentPath;
    QString fetchErr;
    if (res.ok) {
      if (res.body.isEmpty()) {
        fetchErr = QStringLiteral("Empty .torrent response");
      } else {
        QTemporaryFile tmp(QDir::temp().filePath(QStringLiteral("pfd_rss_dl_XXXXXX.torrent")));
        tmp.setAutoRemove(false);
        if (!tmp.open()) {
          fetchErr = QStringLiteral("Failed to open temporary .torrent file");
        } else if (tmp.write(res.body) != res.body.size()) {
          fetchErr = QStringLiteral("Failed to write temporary .torrent file");
        } else {
          tmp.flush();
          torrentPath = tmp.fileName();
        }
        tmp.close();
      }
    } else {
      fetchErr = res.error;
    }

    if (shuttingDown_.load() || windowGuard.isNull()) {
      if (!torrentPath.isEmpty()) {
        QFile::remove(torrentPath);
      }
      QMetaObject::invokeMethod(
          static_cast<QObject*>(app_),
          [this, rssS]() {
            settleRssDownloadIfNeeded(rssS, false);
            rssDownloadPipeline_->finishRssTorrent(
                [this](pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem next) {
                  startOneRssTorrentUrlOnUi(std::move(next));
                });
          },
          Qt::QueuedConnection);
      return;
    }

    const QString pathCopy = torrentPath;
    const QString errCopy = fetchErr;

    QObject* target = static_cast<QObject*>(windowGuard.data());
    QMetaObject::invokeMethod(
        target,
        [this, windowGuard, workerPtr, pathCopy, errCopy, url, resolvedSavePath, useDefaultTrackers,
         defaultTrackersCopy, rssS]() {
          const auto finish = [this]() {
            rssDownloadPipeline_->finishRssTorrent(
                [this](pfd::app::RssDownloadPipeline::RssTorrentUrlQueueItem next) {
                  startOneRssTorrentUrlOnUi(std::move(next));
                });
          };

          if (windowGuard.isNull()) {
            if (!pathCopy.isEmpty()) {
              QFile::remove(pathCopy);
            }
            settleRssDownloadIfNeeded(rssS, false);
            finish();
            return;
          }

          if (pathCopy.isEmpty()) {
            logError(QStringLiteral("RSS 种子下载失败：%1")
                         .arg(errCopy.isEmpty() ? QStringLiteral("unknown") : errCopy));
            settleRssDownloadIfNeeded(rssS, false);
            finish();
            return;
          }

          const auto fileErr = pfd::base::validateTorrentFilePath(pathCopy);
          if (fileErr.hasError()) {
            logError(QStringLiteral("RSS 种子文件被拒绝：%1").arg(fileErr.message()));
            QFile::remove(pathCopy);
            settleRssDownloadIfNeeded(rssS, false);
            finish();
            return;
          }

          const QStringList trackers =
              useDefaultTrackers ? pfd::base::sanitizeTrackers(defaultTrackersCopy) : QStringList{};
          workerPtr->addTorrentFile(pathCopy, resolvedSavePath, trackers);
          logInfo(
              QStringLiteral("RSS torrent URL added: %1 savePath=%2").arg(url, resolvedSavePath));
          QFile::remove(pathCopy);
          settleRssDownloadIfNeeded(rssS, true, resolvedSavePath);
          if (windowGuard) {
            windowGuard->showTransferTab();
          }
          finish();
        },
        Qt::QueuedConnection);
  });

  {
    std::lock_guard<std::mutex> lk(magnetTasksMu_);
    magnetTasks_.push_back(std::move(fut));
  }
}

void AppController::bindUiCallbacks() {
  bindUiAddCallbacks();
  bindUiTaskControlCallbacks();
  bindUiTaskDetailCallbacks();
  bindUiTrackerCallbacks();
  bindUiTaskMetaCallbacks();
  bindUiMiscCallbacks();
}

void AppController::bindUiAddCallbacks() {
  window_->setOnAddMagnet([this](const QString& uri, const QString& savePath) {
    const auto trimmed = uri.trimmed();
    const auto valErr = pfd::base::validateMagnetUri(trimmed);
    if (valErr.hasError()) {
      logError(QStringLiteral("Magnet URI rejected: %1").arg(valErr.message()));
      return;
    }
    if (!savePath.trimmed().isEmpty()) {
      const auto pathErr = pfd::base::validatePath(savePath);
      if (pathErr.hasError()) {
        logError(QStringLiteral("Save path rejected: %1").arg(pathErr.message()));
        return;
      }
    }
    libtorrent::error_code ec;
    const auto atp = libtorrent::parse_magnet_uri(trimmed.toStdString(), ec);
    if (!ec) {
      upsertKnownTaskMagnet(pfd::lt::session_ids::taskIdFromInfoHashes(atp.info_hashes), trimmed);
    }
    enqueueMagnet(trimmed, savePath);
  });

  window_->setOnAddTorrentFile([this](const QString& filePath, const QString& savePath) {
    const auto fileErr = pfd::base::validateTorrentFilePath(filePath);
    if (fileErr.hasError()) {
      logError(QStringLiteral("Torrent file rejected: %1").arg(fileErr.message()));
      return;
    }
    if (!savePath.trimmed().isEmpty()) {
      const auto pathErr = pfd::base::validatePath(savePath);
      if (pathErr.hasError()) {
        logError(QStringLiteral("Save path rejected: %1").arg(pathErr.message()));
        return;
      }
    }
    const QString resolved = savePathPolicy_.resolve(savePath);
    const QStringList trackers =
        autoApplyDefaultTrackers_ ? pfd::base::sanitizeTrackers(defaultTrackers_) : QStringList{};
    worker_->addTorrentFile(filePath, resolved, trackers);
    logInfo(QString("Torrent file added: %1 savePath=%2").arg(filePath, resolved));
  });

  window_->setOnAddTorrentFileRequest([this](
                                          const pfd::ui::MainWindow::AddTorrentFileRequest& req) {
    const auto fileErr = pfd::base::validateTorrentFilePath(req.torrentFilePath);
    if (fileErr.hasError()) {
      logError(QStringLiteral("Torrent file rejected: %1").arg(fileErr.message()));
      return;
    }
    if (!req.ui.savePath.trimmed().isEmpty()) {
      const auto pathErr = pfd::base::validatePath(req.ui.savePath);
      if (pathErr.hasError()) {
        logError(QStringLiteral("Save path rejected: %1").arg(pathErr.message()));
        return;
      }
    }
    const auto catErr = pfd::base::validateCategory(req.ui.category);
    if (catErr.hasError()) {
      logError(QStringLiteral("Category rejected: %1").arg(catErr.message()));
      return;
    }
    const auto tagsErr = pfd::base::validateTagsCsv(req.ui.tagsCsv);
    if (tagsErr.hasError()) {
      logError(QStringLiteral("Tags rejected: %1").arg(tagsErr.message()));
      return;
    }

    pfd::core::TaskAddInput tai;
    tai.name = req.ui.name;
    tai.savePath = req.ui.savePath;
    tai.layout = static_cast<pfd::core::ContentLayout>(static_cast<int>(req.ui.layout));
    tai.startTorrent = req.ui.startTorrent;
    tai.stopCondition = req.ui.stopCondition;
    tai.sequentialDownload = req.ui.sequentialDownload;
    tai.skipHashCheck = req.ui.skipHashCheck;
    tai.addToTopQueue = req.ui.addToTopQueue;
    tai.category = req.ui.category;
    tai.tagsCsv = req.ui.tagsCsv;
    tai.fileWanted = req.ui.fileWanted;
    const auto built = pfd::core::buildTaskAdd(tai, savePathPolicy_);

    const QStringList trackers =
        autoApplyDefaultTrackers_ ? pfd::base::sanitizeTrackers(defaultTrackers_) : QStringList{};

    pfd::lt::SessionWorker::AddTorrentOptions opts;
    opts.file_priorities = built.opts.file_priorities;
    opts.start_torrent = built.opts.start_torrent;
    opts.stop_when_ready = built.opts.stop_when_ready;
    opts.sequential_download = built.opts.sequential_download;
    opts.skip_hash_check = built.opts.skip_hash_check;
    opts.add_to_top_queue = built.opts.add_to_top_queue;
    opts.category = built.opts.category;
    opts.tags_csv = built.opts.tags_csv;

    worker_->addTorrentFileWithOptions(req.torrentFilePath, built.finalSavePath, trackers, opts);
    logInfo(
        QStringLiteral("Torrent submitted with file selection. name=%1 savePath=%2 selected=%3/%4")
            .arg(req.ui.name, built.finalSavePath)
            .arg(req.ui.selectedBytes)
            .arg(req.ui.totalBytes));
  });
}

void AppController::bindUiTaskControlCallbacks() {
  window_->setOnPauseTask([this](const pfd::base::TaskId& taskId) {
    taskControlCommandUseCase_->pauseTask(taskId);
    injectTransitionalStatus(taskId, pfd::base::TaskStatus::kPaused);
  });

  window_->setOnResumeTask([this](const pfd::base::TaskId& taskId) {
    taskControlCommandUseCase_->resumeTask(taskId);
    injectTransitionalStatus(taskId, pfd::base::TaskStatus::kQueued);
  });

  window_->setOnRemoveTask([this](const pfd::base::TaskId& taskId, bool removeFiles) {
    taskControlCommandUseCase_->removeTask(taskId, removeFiles);
  });

  window_->setOnSeedPolicyChanged([this](bool unlimited, double ratio) {
    seedUnlimited_ = unlimited;
    autoStopSeedRatio_ = ratio;
    autoStoppedBySeedRatio_.clear();
    logInfo(QStringLiteral("Seed policy updated. unlimited=%1 targetRatio=%2")
                .arg(unlimited ? QStringLiteral("true") : QStringLiteral("false"))
                .arg(ratio, 0, 'f', 2));
  });

  window_->setOnSetTaskConnectionsLimit([this](const pfd::base::TaskId& taskId, int limit) {
    taskControlCommandUseCase_->setTaskConnectionsLimit(taskId, limit);
  });
  window_->setOnForceStartTask([this](const pfd::base::TaskId& taskId) {
    taskControlCommandUseCase_->forceStartTask(taskId);
  });
  window_->setOnForceRecheckTask([this](const pfd::base::TaskId& taskId) {
    taskControlCommandUseCase_->forceRecheckTask(taskId);
  });
  window_->setOnForceReannounceTask([this](const pfd::base::TaskId& taskId) {
    taskControlCommandUseCase_->forceReannounceTask(taskId);
  });
  window_->setOnSetSequentialDownload([this](const pfd::base::TaskId& taskId, bool enabled) {
    taskControlCommandUseCase_->setSequentialDownload(taskId, enabled);
  });
  window_->setOnSetAutoManagedTask([this](const pfd::base::TaskId& taskId, bool enabled) {
    taskControlCommandUseCase_->setAutoManagedTask(taskId, enabled);
  });
}

void AppController::bindUiTaskDetailCallbacks() {
  window_->setOnQueryCopyPayload([this](const pfd::base::TaskId& taskId) {
    return taskDetailQueryUseCase_->queryCopyPayload(taskId);
  });
  window_->setOnQueryTaskFiles([this](const pfd::base::TaskId& taskId) {
    LOG_DEBUG(QStringLiteral("[main] UI query task files taskId=%1").arg(taskId.toString()));
    return taskDetailQueryUseCase_->queryTaskFiles(taskId);
  });
  window_->setOnSetTaskFilePriority([this](const pfd::base::TaskId& taskId,
                                           const std::vector<int>& fileIndices,
                                           pfd::core::TaskFilePriorityLevel level) {
    LOG_INFO(QStringLiteral("[main] UI set task file priority taskId=%1 level=%2 files=%3")
                 .arg(taskId.toString())
                 .arg(static_cast<int>(level))
                 .arg(fileIndices.size()));
    worker_->setTaskFilePriority(taskId, fileIndices, pfd::app::mapTaskFilePriorityLevel(level));
  });
  window_->setOnRenameTaskFileOrFolder(
      [this](const pfd::base::TaskId& taskId, const QString& logicalPath, const QString& newName) {
        LOG_INFO(QStringLiteral("[main] UI rename file/folder taskId=%1 path=%2 newName=%3")
                     .arg(taskId.toString(), logicalPath, newName));
        worker_->renameTaskFileOrFolder(taskId, logicalPath, newName);
      });
  window_->setOnQueryTaskTrackerSnapshot([this](const pfd::base::TaskId& taskId) {
    LOG_DEBUG(QStringLiteral("[main] UI query task trackers taskId=%1").arg(taskId.toString()));
    return taskDetailQueryUseCase_->queryTaskTrackers(taskId);
  });
  window_->setOnAddTaskTracker([this](const pfd::base::TaskId& taskId, const QString& url) {
    const auto err = pfd::base::validateTrackerUrl(url);
    if (err.hasError()) {
      LOG_WARN(QStringLiteral("[main] UI add tracker rejected taskId=%1 url=%2 reason=%3")
                   .arg(taskId.toString(), url, err.message()));
      return;
    }
    LOG_INFO(QStringLiteral("[main] UI add tracker taskId=%1 url=%2").arg(taskId.toString(), url));
    worker_->addTaskTracker(taskId, url.trimmed());
  });
  window_->setOnEditTaskTracker([this](const pfd::base::TaskId& taskId, const QString& oldUrl,
                                       const QString& newUrl) {
    const auto err = pfd::base::validateTrackerUrl(newUrl);
    if (err.hasError()) {
      LOG_WARN(QStringLiteral("[main] UI edit tracker rejected taskId=%1 old=%2 new=%3 reason=%4")
                   .arg(taskId.toString(), oldUrl, newUrl, err.message()));
      return;
    }
    LOG_INFO(QStringLiteral("[main] UI edit tracker taskId=%1 old=%2 new=%3")
                 .arg(taskId.toString(), oldUrl, newUrl));
    worker_->editTaskTracker(taskId, oldUrl.trimmed(), newUrl.trimmed());
  });
  window_->setOnRemoveTaskTracker([this](const pfd::base::TaskId& taskId, const QString& url) {
    LOG_INFO(
        QStringLiteral("[main] UI remove tracker taskId=%1 url=%2").arg(taskId.toString(), url));
    worker_->removeTaskTracker(taskId, url.trimmed());
  });
  window_->setOnForceReannounceTracker([this](const pfd::base::TaskId& taskId, const QString& url) {
    LOG_INFO(QStringLiteral("[main] UI reannounce tracker taskId=%1 url=%2")
                 .arg(taskId.toString(), url));
    worker_->forceReannounceTracker(taskId, url.trimmed());
  });
  window_->setOnForceReannounceAllTrackers([this](const pfd::base::TaskId& taskId) {
    LOG_INFO(QStringLiteral("[main] UI reannounce all trackers taskId=%1").arg(taskId.toString()));
    worker_->forceReannounceAllTrackers(taskId);
  });

  window_->setOnQueryTaskPeers([this](const pfd::base::TaskId& taskId) {
    return taskDetailQueryUseCase_->queryTaskPeers(taskId);
  });

  window_->setOnQueryTaskWebSeeds([this](const pfd::base::TaskId& taskId) {
    return taskDetailQueryUseCase_->queryTaskWebSeeds(taskId);
  });
}

void AppController::bindUiTrackerCallbacks() {
  window_->setOnEditTrackers([this](const pfd::base::TaskId& taskId, const QStringList& trackers) {
    const auto listErr = pfd::base::validateTrackerList(trackers);
    if (listErr.hasError()) {
      logError(QStringLiteral("Tracker list rejected: %1").arg(listErr.message()));
      return;
    }
    const QStringList clean = pfd::base::sanitizeTrackers(trackers);
    taskTrackers_[taskId.toString(QUuid::WithoutBraces)] = clean;
    worker_->editTrackers(taskId, clean);
    logInfo(QStringLiteral("Trackers updated. taskId=%1 count=%2")
                .arg(taskId.toString())
                .arg(clean.size()));
  });
  window_->setOnQueryTaskTrackers([this](const pfd::base::TaskId& taskId) {
    return taskTrackers_[taskId.toString(QUuid::WithoutBraces)];
  });
  window_->setOnDefaultTrackersChanged([this](bool enabled, const QStringList& trackers) {
    autoApplyDefaultTrackers_ = enabled;
    if (enabled) {
      defaultTrackers_ =
          pfd::base::sanitizeTrackers(pfd::core::ConfigService::normalizeTrackers(trackers));
    }
    saveSettings();
    logInfo(QStringLiteral("Default trackers updated. enabled=%1 count=%2")
                .arg(enabled ? QStringLiteral("true") : QStringLiteral("false"))
                .arg(enabled ? defaultTrackers_.size() : defaultTrackers_.size()));
  });
  window_->setOnQueryDefaultTrackers([this]() { return defaultTrackers_; });
  window_->setOnQueryDefaultTrackerEnabled([this]() { return autoApplyDefaultTrackers_; });
}

void AppController::bindUiTaskMetaCallbacks() {
  window_->setOnMoveTask([this](const pfd::base::TaskId& taskId, const QString& targetPath) {
    if (taskMetaCommandUseCase_ == nullptr ||
        !taskMetaCommandUseCase_->moveTask(taskId, targetPath)) {
      return;
    }
    worker_->moveStorage(taskId, targetPath);
    uiRefreshScheduler_->requestRefresh();
    logInfo(
        QStringLiteral("Move requested. taskId=%1 target=%2").arg(taskId.toString(), targetPath));
  });

  window_->setOnRenameTask([this](const pfd::base::TaskId& taskId, const QString& name) {
    if (taskMetaCommandUseCase_ == nullptr || !taskMetaCommandUseCase_->renameTask(taskId, name)) {
      return;
    }
    uiRefreshScheduler_->requestRefresh();
    logInfo(QStringLiteral("Rename applied. taskId=%1 name=%2").arg(taskId.toString(), name));
  });

  window_->setOnCategoryChanged([this](const pfd::base::TaskId& taskId, const QString& category) {
    if (taskMetaCommandUseCase_ == nullptr ||
        !taskMetaCommandUseCase_->updateCategory(taskId, category)) {
      return;
    }
    uiRefreshScheduler_->requestRefresh();
    logInfo(
        QStringLiteral("Category updated. taskId=%1 category=%2").arg(taskId.toString(), category));
  });

  window_->setOnTagsChanged([this](const pfd::base::TaskId& taskId, const QStringList& tags) {
    if (taskMetaCommandUseCase_ == nullptr || !taskMetaCommandUseCase_->updateTags(taskId, tags)) {
      return;
    }
    uiRefreshScheduler_->requestRefresh();
    logInfo(
        QStringLiteral("Tags updated. taskId=%1 count=%2").arg(taskId.toString()).arg(tags.size()));
  });
}

void AppController::bindUiMiscCallbacks() {
  window_->setOnFirstLastPiecePriority([this](const pfd::base::TaskId& taskId, bool enabled) {
    worker_->setFirstLastPiecePriority(taskId, enabled);
    logInfo(QStringLiteral("首尾块优先下载已%1。taskId=%2")
                .arg(enabled ? QStringLiteral("启用") : QStringLiteral("关闭"), taskId.toString()));
  });
  window_->setOnPreviewFile([this](const pfd::base::TaskId& taskId) {
    const auto snap = pipeline_->snapshot(taskId);
    if (!snap.has_value() || snap->savePath.isEmpty()) {
      logError(QStringLiteral("预览失败：未找到保存路径。"));
      return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(snap->savePath));
    logInfo(QStringLiteral("打开预览：%1").arg(snap->savePath));
  });
  window_->setOnQueuePositionUp(
      [this](const pfd::base::TaskId& taskId) { worker_->queuePositionUp(taskId); });
  window_->setOnQueuePositionDown(
      [this](const pfd::base::TaskId& taskId) { worker_->queuePositionDown(taskId); });
  window_->setOnQueuePositionTop(
      [this](const pfd::base::TaskId& taskId) { worker_->queuePositionTop(taskId); });
  window_->setOnQueuePositionBottom(
      [this](const pfd::base::TaskId& taskId) { worker_->queuePositionBottom(taskId); });
}

void AppController::bindWorkerCallbacks() {
  worker_->setAlertsCallback([this](std::vector<pfd::lt::LtAlertView> views) {
    QMetaObject::invokeMethod(
        static_cast<QObject*>(app_),
        [this, views = std::move(views)]() {
          for (const auto& view : views) {
            if (!view.magnet.isEmpty()) {
              upsertKnownTaskMagnet(view.taskId, view.magnet);
            }
          }
          const auto ingestResult = eventIngestOrchestrator_->ingest(views);
          LOG_DEBUG(QStringLiteral("[main] Consumed alert batch: views=%1 dup=%2 removed=%3")
                        .arg(views.size())
                        .arg(ingestResult.duplicateRejectedTaskIds.size())
                        .arg(ingestResult.removedTaskIds.size()));
          for (const auto& taskId : ingestResult.removedTaskIds) {
            knownTaskMagnets_.erase(taskId.toString(QUuid::WithoutBraces));
          }
          QSet<QString> duplicateUiShown;
          for (const auto& taskId : ingestResult.duplicateRejectedTaskIds) {
            const QString tid = taskId.toString(QUuid::WithoutBraces);
            if (duplicateUiShown.contains(tid)) {
              continue;
            }
            duplicateUiShown.insert(tid);
            QString label;
            if (const auto snap = pipeline_->snapshot(taskId)) {
              if (!snap->name.isEmpty()) {
                label = snap->name;
              }
            }
            window_->notifyTaskAlreadyInList(label);
          }
          if (systemTray_ != nullptr) {
            const auto snapshots = pipeline_->snapshots();
            for (const auto& s : snapshots) {
              if (s.status != pfd::base::TaskStatus::kFinished)
                continue;
              const auto key = s.taskId.toString(QUuid::WithoutBraces);
              if (notifiedFinished_.count(key) > 0)
                continue;
              notifiedFinished_.insert(key);
              const QString name = s.name.isEmpty() ? key.left(8) : s.name;
              systemTray_->showNotification(QStringLiteral("下载完成"), name);
            }
          }
          uiRefreshScheduler_->requestRefresh();
        },
        Qt::QueuedConnection);
  });
}

void AppController::applySeedingPolicy(const std::vector<pfd::core::TaskSnapshot>& snapshots) {
  const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
  for (const auto& s : snapshots) {
    const QString taskKey = s.taskId.toString(QUuid::WithoutBraces);
    if (s.status != pfd::base::TaskStatus::kSeeding) {
      autoStoppedBySeedRatio_.erase(taskKey);
      autoStoppedBySeedTime_.erase(taskKey);
      continue;
    }
    if (seedUnlimited_) {
      continue;
    }
    if (s.downloadedBytes > 0) {
      const double ratio =
          static_cast<double>(s.uploadedBytes) / static_cast<double>(s.downloadedBytes);
      if (ratio >= autoStopSeedRatio_ && autoStoppedBySeedRatio_.insert(taskKey).second) {
        worker_->pauseTask(s.taskId);
        LOG_INFO(QStringLiteral("[main] Auto-stop seeding by ratio. taskId=%1 ratio=%2 target=%3")
                     .arg(s.taskId.toString())
                     .arg(ratio, 0, 'f', 2)
                     .arg(autoStopSeedRatio_, 0, 'f', 2));
        continue;
      }
    }
    if (seedMaxMinutes_ > 0 && s.completedOnMs > 0) {
      const qint64 seedingMinutes = (nowMs - s.completedOnMs) / 60000;
      if (seedingMinutes >= seedMaxMinutes_ && autoStoppedBySeedTime_.insert(taskKey).second) {
        worker_->pauseTask(s.taskId);
        LOG_INFO(QStringLiteral("[main] Auto-stop seeding by time. taskId=%1 minutes=%2 limit=%3")
                     .arg(s.taskId.toString())
                     .arg(seedingMinutes)
                     .arg(seedMaxMinutes_));
      }
    }
  }
  maybeRunPostDownloadAction(snapshots);
}

void AppController::applyTaskMetaUpdate(const pfd::base::TaskId& taskId, const QString& name,
                                        const QString& savePath, const QString& category,
                                        const QString& tagsCsv) {
  if (taskMetaCommandUseCase_ != nullptr) {
    taskMetaCommandUseCase_->upsertMeta(taskId, name, savePath, category, tagsCsv);
  } else {
    pfd::core::TaskEvent ev;
    ev.type = pfd::core::TaskEventType::kUpsertMeta;
    ev.taskId = taskId;
    ev.name = name;
    ev.savePath = savePath;
    ev.category = category;
    ev.tags = tagsCsv;
    pipeline_->consume(ev);
  }
  uiRefreshScheduler_->requestRefresh();
}

void AppController::loadSettings() {
  const auto config = pfd::core::ConfigService::loadCombinedSettings();
  cachedAppSettings_ = config.app;
  autoApplyDefaultTrackers_ = config.controller.auto_apply_default_trackers;
  defaultTrackers_ =
      pfd::core::ConfigService::normalizeTrackers(config.controller.default_trackers);
  magnetMaxInFlight_ = config.controller.magnet_max_inflight;
  bottomStatusEnabled_ = config.controller.bottom_status_enabled;
  bottomStatusRefreshMs_ = config.controller.bottom_status_refresh_ms;
  taskAutoSaveMs_ = config.controller.task_autosave_ms;
  restoreStartPaused_ = config.controller.restore_start_paused;
  logInfo(QStringLiteral("Settings loaded. autoApplyDefaultTrackers=%1 defaultTrackerCount=%2")
              .arg(autoApplyDefaultTrackers_ ? QStringLiteral("true") : QStringLiteral("false"))
              .arg(defaultTrackers_.size()));
}

void AppController::saveSettings() const {
  pfd::core::CombinedSettings config;
  config.app = cachedAppSettings_;
  config.controller.auto_apply_default_trackers = autoApplyDefaultTrackers_;
  config.controller.default_trackers = defaultTrackers_;
  config.controller.magnet_max_inflight = magnetMaxInFlight_;
  config.controller.bottom_status_enabled = bottomStatusEnabled_;
  config.controller.bottom_status_refresh_ms = bottomStatusRefreshMs_;
  config.controller.task_autosave_ms = taskAutoSaveMs_;
  config.controller.restore_start_paused = restoreStartPaused_;
  const auto result = pfd::core::ConfigService::saveCombinedSettings(config);
  if (!result.ok) {
    logError(QStringLiteral("保存配置失败：%1").arg(result.message));
  }
}

void AppController::upsertKnownTaskMagnet(const pfd::base::TaskId& taskId, const QString& magnet) {
  if (taskId.isNull()) {
    return;
  }
  const QString trimmed = magnet.trimmed();
  if (trimmed.isEmpty()) {
    return;
  }
  knownTaskMagnets_[taskId.toString(QUuid::WithoutBraces)] = trimmed;
}

bool AppController::restoreStartPaused() const {
  return restoreStartPaused_;
}

void AppController::savePersistedTasks() const {
  const QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  QDir().mkpath(configPath);
  const QString filePath = QDir(configPath).filePath(QStringLiteral("tasks.json"));
  const auto snapshots = pipeline_->snapshots();

  QJsonArray arr;
  for (const auto& s : snapshots) {
    const QString taskKey = s.taskId.toString(QUuid::WithoutBraces);
    QJsonObject o;
    o.insert(QStringLiteral("task_id"), taskKey);
    const auto it = knownTaskMagnets_.find(taskKey);
    if (it != knownTaskMagnets_.end() && !it->second.trimmed().isEmpty()) {
      o.insert(QStringLiteral("magnet"), it->second);
    }
    o.insert(QStringLiteral("name"), s.name);
    o.insert(QStringLiteral("save_path"), s.savePath);
    o.insert(QStringLiteral("category"), s.category);
    o.insert(QStringLiteral("tags"), s.tags);
    o.insert(QStringLiteral("status"), static_cast<int>(s.status));
    o.insert(QStringLiteral("last_updated_ms"), static_cast<qint64>(s.lastUpdatedMs));
    arr.push_back(o);
  }

  const QByteArray tasksPayload = QJsonDocument(arr).toJson(QJsonDocument::Compact);

  QJsonObject envelope;
  envelope.insert(QStringLiteral("version"), kTasksJsonVersion);
  envelope.insert(QStringLiteral("checksum"), pfd::base::sha256Hex(tasksPayload));
  envelope.insert(QStringLiteral("tasks"), arr);

  const QJsonDocument doc(envelope);
  const auto err =
      pfd::base::writeWholeFileWithBackup(filePath, doc.toJson(QJsonDocument::Indented));
  if (err.hasError()) {
    LOG_ERROR(QStringLiteral("[main] save persisted tasks failed: %1 err=%2")
                  .arg(filePath, err.message()));
  }
}

void AppController::saveResumeData() const {
  const QString rd = pfd::base::resumeDir();
  if (rd.isEmpty()) {
    return;
  }
  if (!pfd::base::ensureExists(rd)) {
    LOG_WARN(QStringLiteral("[main] Failed to ensure resume data dir exists: %1").arg(rd));
  }
  const int saved = worker_->saveAllResumeData(rd);
  LOG_INFO(QStringLiteral("[main] Resume data saved: %1 files").arg(saved));

  // Remove stale resume-data files for tasks that no longer exist.
  std::unordered_set<QString> activeTaskKeys;
  const auto snapshots = pipeline_->snapshots();
  activeTaskKeys.reserve(snapshots.size());
  for (const auto& s : snapshots) {
    activeTaskKeys.insert(s.taskId.toString(QUuid::WithoutBraces));
  }
  QDirIterator it(rd, {QStringLiteral("*.rd")}, QDir::Files);
  while (it.hasNext()) {
    it.next();
    const QString taskKey = it.fileInfo().completeBaseName();
    if (activeTaskKeys.count(taskKey) == 0) {
      QFile::remove(it.filePath());
    }
  }
}

void AppController::loadPersistedTasks() {
  const QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  const QString filePath = QDir(configPath).filePath(QStringLiteral("tasks.json"));

  // Phase 1: load tasks.json with integrity check + backup recovery
  std::unordered_map<QString, QJsonObject> metaByTaskKey;
  {
    const auto result = pfd::base::readWholeFileWithRecovery(filePath);
    if (result.error.hasError() || result.data.isEmpty()) {
      if (result.error.hasError() && result.error.code() != pfd::base::ErrorCode::kFileNotFound) {
        LOG_ERROR(QStringLiteral("[main] Failed to read tasks.json (all copies): %1")
                      .arg(result.error.message()));
      }
    } else {
      if (result.actualPath != filePath) {
        LOG_WARN(QStringLiteral("[main] Primary tasks.json unreadable, recovered from: %1")
                     .arg(result.actualPath));
      }

      const auto doc = QJsonDocument::fromJson(result.data);
      QJsonArray tasksArray;
      bool integrityOk = true;

      if (doc.isObject()) {
        const QJsonObject envelope = doc.object();
        const int version = envelope.value(QStringLiteral("version")).toInt(0);
        if (version < 1 || version > kTasksJsonVersion) {
          LOG_WARN(QStringLiteral("[main] tasks.json version mismatch: got %1, expected <=%2")
                       .arg(version)
                       .arg(kTasksJsonVersion));
        }
        tasksArray = envelope.value(QStringLiteral("tasks")).toArray();
        const QString storedChecksum =
            envelope.value(QStringLiteral("checksum")).toString().trimmed();
        if (!storedChecksum.isEmpty()) {
          const QByteArray tasksPayload = QJsonDocument(tasksArray).toJson(QJsonDocument::Compact);
          const QString computed = pfd::base::sha256Hex(tasksPayload);
          if (computed != storedChecksum) {
            LOG_WARN(QStringLiteral("[main] tasks.json checksum mismatch "
                                    "(stored=%1 computed=%2), data may be corrupt")
                         .arg(storedChecksum, computed));
            integrityOk = false;
          }
        }
      } else if (doc.isArray()) {
        // Legacy format (bare array, no envelope) — accept without checksum
        tasksArray = doc.array();
        LOG_INFO(
            QStringLiteral("[main] tasks.json legacy format detected, will upgrade on next save."));
      }

      if (integrityOk) {
        for (const auto& v : tasksArray) {
          if (!v.isObject())
            continue;
          const QJsonObject o = v.toObject();
          const QString key = o.value(QStringLiteral("task_id")).toString().trimmed();
          if (!key.isEmpty()) {
            metaByTaskKey[key] = o;
          }
        }
      } else {
        LOG_ERROR(QStringLiteral("[main] tasks.json integrity check failed, skipping load. "
                                 "Backup recovery will be attempted on next read."));
      }
    }
  }

  // Phase 2: scan resume directory for .rd files (primary restore path)
  const QString rd = pfd::base::resumeDir();
  std::unordered_set<QString> restoredFromResumeData;
  int restoredRd = 0;
  if (!rd.isEmpty() && QDir(rd).exists()) {
    QDirIterator it(rd, {QStringLiteral("*.rd")}, QDir::Files);
    while (it.hasNext()) {
      it.next();
      const QString taskKey = it.fileInfo().completeBaseName();
      // Only restore resume data entries that are present in tasks.json metadata.
      // This prevents deleted tasks with stale .rd files from reappearing.
      if (metaByTaskKey.count(taskKey) == 0) {
        continue;
      }
      auto [data, err] = pfd::base::readWholeFile(it.filePath());
      if (err.hasError() || data.isEmpty())
        continue;

      bool paused = restoreStartPaused();
      auto metaIt = metaByTaskKey.find(taskKey);
      if (metaIt != metaByTaskKey.end()) {
        const int savedStatus = metaIt->second.value(QStringLiteral("status")).toInt(-1);
        if (savedStatus >= 0) {
          const auto st = static_cast<pfd::base::TaskStatus>(savedStatus);
          paused = (st == pfd::base::TaskStatus::kPaused ||
                    st == pfd::base::TaskStatus::kFinished || st == pfd::base::TaskStatus::kError);
        }
      }
      worker_->addFromResumeData(data, paused);
      restoredFromResumeData.insert(taskKey);

      if (metaIt != metaByTaskKey.end()) {
        const auto& o = metaIt->second;
        const QString name = o.value(QStringLiteral("name")).toString().trimmed();
        const QString savePath = o.value(QStringLiteral("save_path")).toString().trimmed();
        const QString category = o.value(QStringLiteral("category")).toString().trimmed();
        const QString tags = o.value(QStringLiteral("tags")).toString().trimmed();
        const QString magnet = o.value(QStringLiteral("magnet")).toString().trimmed();
        const auto taskId = QUuid::fromString(QStringLiteral("{%1}").arg(taskKey));
        if (!taskId.isNull()) {
          if (!magnet.isEmpty())
            upsertKnownTaskMagnet(taskId, magnet);
          applyTaskMetaUpdate(taskId, name, savePath, category, tags);
        }
      }
      ++restoredRd;
    }
  }

  // Phase 3: fallback to magnet URI for tasks without resume data
  int restoredMagnet = 0;
  for (const auto& [key, o] : metaByTaskKey) {
    if (restoredFromResumeData.count(key) > 0)
      continue;

    const QString magnet = o.value(QStringLiteral("magnet")).toString().trimmed();
    const QString savePath = o.value(QStringLiteral("save_path")).toString().trimmed();
    const QString name = o.value(QStringLiteral("name")).toString().trimmed();
    const QString category = o.value(QStringLiteral("category")).toString().trimmed();
    const QString tags = o.value(QStringLiteral("tags")).toString().trimmed();
    if (magnet.isEmpty())
      continue;

    libtorrent::error_code ec;
    const auto atp = libtorrent::parse_magnet_uri(magnet.toStdString(), ec);
    if (ec)
      continue;

    const auto taskId = pfd::lt::session_ids::taskIdFromInfoHashes(atp.info_hashes);
    upsertKnownTaskMagnet(taskId, magnet);

    const int savedStatus = o.value(QStringLiteral("status")).toInt(-1);
    const bool shouldPause =
        savedStatus >= 0
            ? (static_cast<pfd::base::TaskStatus>(savedStatus) == pfd::base::TaskStatus::kPaused ||
               static_cast<pfd::base::TaskStatus>(savedStatus) ==
                   pfd::base::TaskStatus::kFinished ||
               static_cast<pfd::base::TaskStatus>(savedStatus) == pfd::base::TaskStatus::kError)
            : restoreStartPaused();

    worker_->addMagnet(magnet, savePathPolicy_.resolve(savePath));
    if (shouldPause) {
      QTimer::singleShot(1500, static_cast<QObject*>(app_),
                         [this, taskId]() { worker_->pauseTask(taskId); });
    }
    applyTaskMetaUpdate(taskId, name, savePath, category, tags);
    ++restoredMagnet;
  }

  const int total = restoredRd + restoredMagnet;
  if (total > 0) {
    logInfo(QStringLiteral("已恢复任务 %1 个（resume_data: %2, magnet: %3）。")
                .arg(total)
                .arg(restoredRd)
                .arg(restoredMagnet));
  }
}

void AppController::injectTransitionalStatus(const pfd::base::TaskId& taskId,
                                             pfd::base::TaskStatus status) {
  pfd::core::TaskEvent ev;
  ev.type = pfd::core::TaskEventType::kStatusChanged;
  ev.taskId = taskId;
  ev.status = status;
  pipeline_->consume(ev);

  if (eventIngestOrchestrator_ != nullptr) {
    eventIngestOrchestrator_->lockProgressStatus(taskId,
                                                 QDateTime::currentMSecsSinceEpoch() + 1000);
  }

  uiRefreshScheduler_->requestRefresh();
}

void AppController::logInfo(const QString& msg) const {
  window_->appendLog(msg);
  LOG_INFO(QStringLiteral("[main] %1").arg(msg));
}

void AppController::logError(const QString& msg) const {
  window_->appendLog(msg);
  LOG_ERROR(QStringLiteral("[main] %1").arg(msg));
}

void AppController::initializeRss() {
  const QString rssDataDir =
      QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))
          .filePath(QStringLiteral("rss"));
  rssService_ = std::make_unique<pfd::core::rss::RssService>(rssDataDir);
  rssService_->loadState();
  rssService_->setRequestHeaders(pfd::core::rss::RssFetcher::RequestHeaders{
      cachedAppSettings_.http_user_agent, cachedAppSettings_.http_accept_language,
      cachedAppSettings_.http_cookie_header, cachedAppSettings_.http_cookie_rules});

  rssService_->setDownloadRequestCallback([this](const pfd::core::rss::AutoDownloadRequest& req) {
    if (shuttingDown_.load())
      return;
    if (!req.magnet.isEmpty()) {
      enqueueMagnet(req.magnet, req.save_path, req.rss_settlement,
                    req.add_without_interactive_confirm, req.category, req.tags_csv);
    } else if (!req.torrent_url.isEmpty()) {
      enqueueRssTorrentUrl(req.torrent_url, req.save_path, req.referer_url, req.rss_settlement);
    } else {
      logError(QStringLiteral("[rss-dl] No magnet or torrent URL for \"%1\"")
                   .arg(req.item_title.left(120)));
      return;
    }
    logInfo(QStringLiteral("[rss-dl] \"%1\" rule=%2 feed=%3 item=%4")
                .arg(req.item_title.left(60), req.rule_id.left(8), req.feed_id.left(8),
                     req.item_id.left(8)));
  });

  window_->setRssService(rssService_.get());

  window_->setSearchDataSources(
      [this]() { return pipeline_->snapshots(); },
      [this](const QString& keyword) -> std::vector<std::pair<QString, QString>> {
        std::vector<std::pair<QString, QString>> out;
        if (!rssService_)
          return out;
        const auto items = rssService_->items();
        for (const auto& item : items) {
          if (item.title.contains(keyword, Qt::CaseInsensitive)) {
            out.emplace_back(item.title, item.link);
          }
        }
        return out;
      });
  window_->setSearchRequestHeaders(pfd::ui::SearchTab::RequestHeaders{
      cachedAppSettings_.http_user_agent, cachedAppSettings_.http_accept_language,
      cachedAppSettings_.http_cookie_header});

  QObject::connect(window_, &pfd::ui::MainWindow::rssSettingsChanged, static_cast<QObject*>(app_),
                   [this]() { updateRssTimerInterval(); });

  const int intervalMs = rssService_->settings().refresh_interval_minutes * 60 * 1000;
  rssTimer_ = new QTimer(static_cast<QObject*>(app_));
  rssTimer_->setInterval(intervalMs);
  QObject::connect(rssTimer_, &QTimer::timeout, static_cast<QObject*>(app_), [this]() {
    if (shuttingDown_.load())
      return;
    rssService_->refreshAllFeeds();
    rssService_->saveState();
  });
  rssTimer_->start();
}

}  // namespace pfd::app
