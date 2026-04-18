#ifndef PFD_APP_CONTROLLER_H
#define PFD_APP_CONTROLLER_H

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <atomic>
#include <deque>
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

#include "core/rss/rss_service.h"
#include "core/save_path_policy.h"
#include "core/task_pipeline_service.h"
#include "lt/session_worker.h"
#include "ui/main_window.h"
#include "ui/system_tray.h"

class QTimer;

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
  struct MagnetQueueItem {
    QString uri;
    QString savePath;
    pfd::core::rss::RssDownloadSettlement rssSettlement;
    bool skipInteractiveAdd{false};
    QString category;
    QString tagsCsv;
  };

  void updateRssTimerInterval();
  void loadSettings();
  void saveSettings() const;
  void bindUiCallbacks();
  void bindWorkerCallbacks();
  void scheduleTimedAction();
  void executeTimedAction();
  void applySeedingPolicy(const std::vector<pfd::core::TaskSnapshot>& snapshots);
  void applyTaskMetaUpdate(const pfd::base::TaskId& taskId, const QString& name,
                           const QString& savePath, const QString& category,
                           const QString& tagsCsv);
  void applyRuntimeSettingsFromConfig(const pfd::core::AppSettings* app_settings = nullptr);
  void applyBuiltinHttpTrackerFromSettings(const pfd::core::AppSettings& s);
  void loadPersistedTasks();
  void savePersistedTasks() const;
  void saveResumeData() const;
  void schedulePersistedTasksAutoSave();
  void initializeRss();
  void settleRssDownloadIfNeeded(const pfd::core::rss::RssDownloadSettlement& s, bool ok,
                                 const QString& resolved_save_override = {});
  void upsertKnownTaskMagnet(const pfd::base::TaskId& taskId, const QString& magnet);
  void injectTransitionalStatus(const pfd::base::TaskId& taskId, pfd::base::TaskStatus status);
  [[nodiscard]] bool restoreStartPaused() const;
  void logInfo(const QString& msg) const;
  void logError(const QString& msg) const;

  void enqueueMagnet(const QString& uri, const QString& savePath,
                     const pfd::core::rss::RssDownloadSettlement& rssSettlement = {},
                     bool skipInteractiveAdd = false, const QString& category = {},
                     const QString& tagsCsv = {});
  void pumpMagnetQueueOnUi();
  void startOneMagnetOnUi(MagnetQueueItem item);

  struct RssTorrentUrlQueueItem {
    QString url;
    QString savePath;
    QString referer;
    pfd::core::rss::RssDownloadSettlement rssSettlement;
  };
  void enqueueRssTorrentUrl(const QString& url, const QString& savePath,
                            const QString& referer = {},
                            const pfd::core::rss::RssDownloadSettlement& rssSettlement = {});
  void pumpRssTorrentUrlQueueOnUi();
  void startOneRssTorrentUrlOnUi(RssTorrentUrlQueueItem item);

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

  std::mutex magnetMu_;
  std::deque<MagnetQueueItem> magnetQueue_;
  int magnetInFlight_{0};
  int magnetMaxInFlight_{8};  // 同时最多弹出 5-10 个，默认取 8
  bool magnetPumpScheduled_{false};

  std::mutex rssTorrentMu_;
  std::deque<RssTorrentUrlQueueItem> rssTorrentQueue_;
  int rssTorrentInFlight_{0};
  int rssTorrentMaxInFlight_{8};
  bool rssTorrentPumpScheduled_{false};

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

  std::unique_ptr<pfd::core::BuiltinHttpTracker> builtinHttpTracker_;
  std::unique_ptr<pfd::core::rss::RssService> rssService_;
  QTimer* rssTimer_{nullptr};
  QTimer* timedActionTimer_{nullptr};

  pfd::ui::SystemTray* systemTray_{nullptr};
  std::unordered_set<QString> notifiedFinished_;

  QTimer* refreshThrottle_{nullptr};
  bool refreshPending_{false};

  std::unordered_map<QString, qint64> statusLockUntilMs_;
};

}  // namespace pfd::app

#endif  // PFD_APP_CONTROLLER_H
