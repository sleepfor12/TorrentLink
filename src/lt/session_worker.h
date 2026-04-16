#ifndef PFD_LT_SESSION_WORKER_H
#define PFD_LT_SESSION_WORKER_H

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <cstdint>
#include <functional>
#include <vector>

#include "base/types.h"
#include "lt/task_event_mapper.h"

namespace pfd::lt {

class SessionWorker {
public:
  enum class FilePriorityLevel : std::uint8_t {
    kNotDownloaded = 0,
    kDoNotDownload,
    kNormal,
    kHigh,
    kHighest,
    kFileOrder,
  };

  struct FileEntrySnapshot {
    int fileIndex{-1};
    QString logicalPath;
    qint64 sizeBytes{0};
    qint64 downloadedBytes{0};
    double progress01{0.0};
    double availability{-1.0};
    FilePriorityLevel priority{FilePriorityLevel::kNormal};
  };

  enum class TrackerStatus : std::uint8_t {
    kCannotConnect = 0,
    kNotWorking,
    kWorking,
    kUpdating,
  };

  struct TrackerEndpointSnapshot {
    QString ip;
    int port{0};
    QString btProtocol;
    TrackerStatus status{TrackerStatus::kNotWorking};
    int users{-1};
    int seeds{-1};
    int downloads{-1};
    int nextAnnounceSec{-1};
    int minAnnounceSec{-1};
  };

  struct TrackerRowSnapshot {
    QString url;
    int tier{0};
    QString btProtocol;
    TrackerStatus status{TrackerStatus::kNotWorking};
    int users{-1};
    int seeds{-1};
    int downloads{-1};
    int nextAnnounceSec{-1};
    int minAnnounceSec{-1};
    bool readOnly{false};
    std::vector<TrackerEndpointSnapshot> endpoints;
  };

  struct TaskTrackerSnapshot {
    std::vector<TrackerRowSnapshot> fixedRows;
    std::vector<TrackerRowSnapshot> trackers;
  };

  struct CopyPayload {
    QString infoHashV1;
    QString infoHashV2;
    QString magnet;
    QString torrentId;
    QString comment;
  };

  using AlertsCallback = std::function<void(std::vector<LtAlertView>)>;

  struct AddTorrentOptions {
    // 0=不下载；4=默认优先级（libtorrent::default_priority）
    std::vector<std::uint8_t> file_priorities;
    bool start_torrent{true};
    bool stop_when_ready{false};
    bool sequential_download{false};
    bool skip_hash_check{false};
    bool add_to_top_queue{false};
    QString category;
    QString tags_csv;
  };

  struct MagnetMetadata {
    pfd::base::TaskId taskId;
    QString name;
    qint64 totalBytes{0};
    qint64 creationDate{0};
    QString infoHashV1;
    QString infoHashV2;
    QString comment;
    std::vector<QString> filePaths;
    std::vector<qint64> fileSizes;
  };

  struct PeerSnapshot {
    QString ip;
    int port{0};
    QString client;
    double progress{0.0};
    qint64 downloadRate{0};
    qint64 uploadRate{0};
    qint64 totalDownloaded{0};
    qint64 totalUploaded{0};
    QString flags;
    QString connection;
  };

  struct WebSeedSnapshot {
    QString url;
    QString type;  // "BEP19" (url_seed) or "BEP17" (http_seed)
  };

  struct SessionStats {
    int dht_nodes{-1};
    qint64 download_rate{0};  // bytes/s
    qint64 upload_rate{0};    // bytes/s
  };

  SessionWorker();
  ~SessionWorker();

  SessionWorker(const SessionWorker&) = delete;
  SessionWorker& operator=(const SessionWorker&) = delete;

  void setAlertsCallback(AlertsCallback cb);

  void addMagnet(const QString& uri, const QString& savePath, const QStringList& trackers = {});
  // 预取磁力链接元数据（用于弹出文件选择页），超时返回空；成功返回文件列表等信息。
  [[nodiscard]] std::optional<MagnetMetadata>
  prepareMagnetMetadata(const QString& uri, const QString& tempSavePath, int timeout_ms);
  // 用户取消选择页后，清理临时磁力任务。
  void cancelPreparedMagnet(const pfd::base::TaskId& taskId);
  // 用户确认选择页后，使用已缓存的 metadata 重新添加并应用选项与文件优先级。
  void finalizePreparedMagnet(const pfd::base::TaskId& taskId, const QString& savePath,
                              const QStringList& trackers, const AddTorrentOptions& opts);
  void addTorrentFile(const QString& filePath, const QString& savePath,
                      const QStringList& trackers = {});
  void addTorrentFileWithOptions(const QString& filePath, const QString& savePath,
                                 const QStringList& trackers, const AddTorrentOptions& opts);
  void pauseTask(const pfd::base::TaskId& taskId);
  void resumeTask(const pfd::base::TaskId& taskId);
  void removeTask(const pfd::base::TaskId& taskId, bool removeFiles);
  void moveStorage(const pfd::base::TaskId& taskId, const QString& targetPath);
  void editTrackers(const pfd::base::TaskId& taskId, const QStringList& trackers);
  void forceRecheckTask(const pfd::base::TaskId& taskId);
  void forceReannounceTask(const pfd::base::TaskId& taskId);
  void setSequentialDownload(const pfd::base::TaskId& taskId, bool enabled);
  void setAutoManagedTask(const pfd::base::TaskId& taskId, bool enabled);
  void forceStartTask(const pfd::base::TaskId& taskId);
  void setFirstLastPiecePriority(const pfd::base::TaskId& taskId, bool enabled);
  void queuePositionUp(const pfd::base::TaskId& taskId);
  void queuePositionDown(const pfd::base::TaskId& taskId);
  void queuePositionTop(const pfd::base::TaskId& taskId);
  void queuePositionBottom(const pfd::base::TaskId& taskId);

  void applyRuntimeSettings(int download_rate_limit_kib, int upload_rate_limit_kib,
                            int connections_limit, int upload_slots_limit,
                            int per_torrent_upload_slots_limit, int listen_port,
                            bool listen_port_forwarding, int active_downloads, int active_seeds,
                            int active_limit, int max_active_checking,
                            bool auto_manage_prefer_seeds, bool dont_count_slow_torrents,
                            int slow_torrent_dl_rate_threshold, int slow_torrent_ul_rate_threshold,
                            int slow_torrent_inactive_timer, bool enable_dht, bool enable_upnp,
                            bool enable_natpmp, bool enable_lsd, bool proxy_enabled,
                            const QString& proxy_type, const QString& proxy_host, int proxy_port,
                            const QString& proxy_username, const QString& proxy_password,
                            const QString& encryption_mode);

  // 默认对新加入的 torrent 生效；并会尽量应用到已存在的 torrents。
  void setDefaultPerTorrentConnectionsLimit(int per_torrent_connections_limit);

  // 对单个任务设置连接数上限（0 表示不限制）；如果该任务尚未拿到 handle，会延后应用。
  void setTaskConnectionsLimit(const pfd::base::TaskId& taskId, int limit);
  void addFromResumeData(const QByteArray& resumeData, bool startPaused);

  /// Requests all active torrents to save resume data to resumeDir.
  /// Returns the number of files written. Blocks until all are done.
  [[nodiscard]] int saveAllResumeData(const QString& resumeDir);

  [[nodiscard]] CopyPayload queryTaskCopyPayload(const pfd::base::TaskId& taskId);
  [[nodiscard]] SessionStats querySessionStats();
  [[nodiscard]] std::vector<FileEntrySnapshot> queryTaskFiles(const pfd::base::TaskId& taskId);
  [[nodiscard]] TaskTrackerSnapshot queryTaskTrackers(const pfd::base::TaskId& taskId);
  [[nodiscard]] std::vector<PeerSnapshot> queryTaskPeers(const pfd::base::TaskId& taskId);
  [[nodiscard]] std::vector<WebSeedSnapshot> queryTaskWebSeeds(const pfd::base::TaskId& taskId);
  void setTaskFilePriority(const pfd::base::TaskId& taskId, const std::vector<int>& fileIndices,
                           FilePriorityLevel level);
  void renameTaskFileOrFolder(const pfd::base::TaskId& taskId, const QString& logicalPath,
                              const QString& newName);
  void addTaskTracker(const pfd::base::TaskId& taskId, const QString& url);
  void editTaskTracker(const pfd::base::TaskId& taskId, const QString& oldUrl,
                       const QString& newUrl);
  void removeTaskTracker(const pfd::base::TaskId& taskId, const QString& url);
  void forceReannounceTracker(const pfd::base::TaskId& taskId, const QString& url);
  void forceReannounceAllTrackers(const pfd::base::TaskId& taskId);

private:
  struct Impl;
  Impl* impl_;
};

}  // namespace pfd::lt

#endif  // PFD_LT_SESSION_WORKER_H
