#ifndef PFD_LT_SESSION_CMDS_H
#define PFD_LT_SESSION_CMDS_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <future>
#include <memory>
#include <optional>
#include <variant>

#include "base/types.h"
#include "lt/session_worker.h"

namespace pfd::lt::session_cmds {

struct AddMagnetCmd {
  QString uri;
  QString savePath;
  QStringList trackers;
};

struct PrepareMagnetMetadataCmd {
  QString uri;
  QString tempSavePath;
  int timeout_ms{8000};
  std::shared_ptr<std::promise<std::optional<SessionWorker::MagnetMetadata>>> done;
};

struct CancelPreparedMagnetCmd {
  pfd::base::TaskId taskId;
};

struct FinalizePreparedMagnetCmd {
  pfd::base::TaskId taskId;
  QString savePath;
  QStringList trackers;
  SessionWorker::AddTorrentOptions opts;
};

struct AddTorrentFileCmd {
  QString filePath;
  QString savePath;
  QStringList trackers;
  SessionWorker::AddTorrentOptions opts;
  bool has_opts{false};
};

struct PauseCmd {
  pfd::base::TaskId taskId;
};

struct ResumeCmd {
  pfd::base::TaskId taskId;
};

struct RemoveCmd {
  pfd::base::TaskId taskId;
  bool removeFiles{false};
};

struct MoveStorageCmd {
  pfd::base::TaskId taskId;
  QString targetPath;
};

struct EditTrackersCmd {
  pfd::base::TaskId taskId;
  QStringList trackers;
};

struct ForceRecheckCmd {
  pfd::base::TaskId taskId;
};

struct ForceReannounceCmd {
  pfd::base::TaskId taskId;
};

struct SetSequentialDownloadCmd {
  pfd::base::TaskId taskId;
  bool enabled{false};
};

struct SetAutoManagedCmd {
  pfd::base::TaskId taskId;
  bool enabled{true};
};

struct ForceStartCmd {
  pfd::base::TaskId taskId;
};

struct SetFirstLastPiecePriorityCmd {
  pfd::base::TaskId taskId;
  bool enabled{true};
};

struct QueuePositionUpCmd {
  pfd::base::TaskId taskId;
};

struct QueuePositionDownCmd {
  pfd::base::TaskId taskId;
};

struct QueuePositionTopCmd {
  pfd::base::TaskId taskId;
};

struct QueuePositionBottomCmd {
  pfd::base::TaskId taskId;
};

struct ApplyRuntimeSettingsCmd {
  int download_rate_limit_kib{0};
  int upload_rate_limit_kib{0};
  int connections_limit{0};
  int upload_slots_limit{0};
  int per_torrent_upload_slots_limit{0};
  int listen_port{0};
  bool listen_port_forwarding{true};
  int active_downloads{0};
  int active_seeds{0};
  int active_limit{0};
  int max_active_checking{1};
  bool auto_manage_prefer_seeds{false};
  bool dont_count_slow_torrents{true};
  int slow_torrent_dl_rate_threshold{2};
  int slow_torrent_ul_rate_threshold{2};
  int slow_torrent_inactive_timer{60};
  bool enable_dht{true};
  bool enable_upnp{true};
  bool enable_natpmp{true};
  bool enable_lsd{true};
  bool proxy_enabled{false};
  QString proxy_type{QStringLiteral("socks5")};
  QString proxy_host;
  int proxy_port{1080};
  QString proxy_username;
  QString proxy_password;
  QString encryption_mode{QStringLiteral("enabled")};
};

struct SetDefaultPerTorrentConnectionsLimitCmd {
  int limit{0};
};

struct SetTaskConnectionsLimitCmd {
  pfd::base::TaskId taskId;
  int limit{0};
};

struct QueryTaskCopyPayloadCmd {
  pfd::base::TaskId taskId;
  std::shared_ptr<std::promise<SessionWorker::CopyPayload>> done;
};

struct QuerySessionStatsCmd {
  std::shared_ptr<std::promise<SessionWorker::SessionStats>> done;
};

struct QueryTaskFilesCmd {
  pfd::base::TaskId taskId;
  std::shared_ptr<std::promise<std::vector<SessionWorker::FileEntrySnapshot>>> done;
};

struct QueryTaskTrackersCmd {
  pfd::base::TaskId taskId;
  std::shared_ptr<std::promise<SessionWorker::TaskTrackerSnapshot>> done;
};

struct QueryTaskPeersCmd {
  pfd::base::TaskId taskId;
  std::shared_ptr<std::promise<std::vector<SessionWorker::PeerSnapshot>>> done;
};

struct QueryTaskWebSeedsCmd {
  pfd::base::TaskId taskId;
  std::shared_ptr<std::promise<std::vector<SessionWorker::WebSeedSnapshot>>> done;
};

struct AddTaskTrackerCmd {
  pfd::base::TaskId taskId;
  QString url;
};

struct EditTaskTrackerCmd {
  pfd::base::TaskId taskId;
  QString oldUrl;
  QString newUrl;
};

struct RemoveTaskTrackerCmd {
  pfd::base::TaskId taskId;
  QString url;
};

struct ForceReannounceTrackerCmd {
  pfd::base::TaskId taskId;
  QString url;
};

struct ForceReannounceAllTrackersCmd {
  pfd::base::TaskId taskId;
};

struct SetTaskFilePriorityCmd {
  pfd::base::TaskId taskId;
  std::vector<int> fileIndices;
  SessionWorker::FilePriorityLevel level{SessionWorker::FilePriorityLevel::kNormal};
};

struct RenameTaskFileOrFolderCmd {
  pfd::base::TaskId taskId;
  QString logicalPath;
  QString newName;
};

struct AddFromResumeDataCmd {
  QByteArray resumeData;
  bool startPaused{false};
};

struct SaveAllResumeDataCmd {
  QString resumeDir;
  std::shared_ptr<std::promise<int>> done;
};

struct ShutdownCmd {};

using Cmd = std::variant<
    AddMagnetCmd, AddTorrentFileCmd, AddFromResumeDataCmd, PauseCmd, ResumeCmd, RemoveCmd,
    MoveStorageCmd, EditTrackersCmd, ForceRecheckCmd, ForceReannounceCmd, SetSequentialDownloadCmd,
    SetAutoManagedCmd, ForceStartCmd, SetFirstLastPiecePriorityCmd, QueuePositionUpCmd,
    QueuePositionDownCmd, QueuePositionTopCmd, QueuePositionBottomCmd, ApplyRuntimeSettingsCmd,
    SetDefaultPerTorrentConnectionsLimitCmd, SetTaskConnectionsLimitCmd, QueryTaskCopyPayloadCmd,
    QuerySessionStatsCmd, QueryTaskFilesCmd, QueryTaskTrackersCmd, QueryTaskPeersCmd,
    QueryTaskWebSeedsCmd, SetTaskFilePriorityCmd, RenameTaskFileOrFolderCmd, AddTaskTrackerCmd,
    EditTaskTrackerCmd, RemoveTaskTrackerCmd, ForceReannounceTrackerCmd,
    ForceReannounceAllTrackersCmd, SaveAllResumeDataCmd, PrepareMagnetMetadataCmd,
    CancelPreparedMagnetCmd, FinalizePreparedMagnetCmd, ShutdownCmd>;

}  // namespace pfd::lt::session_cmds

#endif  // PFD_LT_SESSION_CMDS_H
