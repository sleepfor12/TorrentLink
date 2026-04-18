#include "lt/session_worker.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QUuid>

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/alert.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/announce_entry.hpp>
#include <libtorrent/download_priority.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/session_stats.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/torrent_flags.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/write_resume_data.hpp>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <future>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <variant>

#include "core/logger.h"
#include "lt/add_torrent_torrent_info_ptr.h"
#include "lt/session_cmds.h"
#include "lt/session_ids.h"
#include "lt/session_ops.h"
#include "lt/task_alert_adapter.h"

namespace pfd::lt {

struct SessionWorker::Impl {
  using AddMagnetCmd = session_cmds::AddMagnetCmd;
  using PrepareMagnetMetadataCmd = session_cmds::PrepareMagnetMetadataCmd;
  using CancelPreparedMagnetCmd = session_cmds::CancelPreparedMagnetCmd;
  using FinalizePreparedMagnetCmd = session_cmds::FinalizePreparedMagnetCmd;
  using AddTorrentFileCmd = session_cmds::AddTorrentFileCmd;
  using AddFromResumeDataCmd = session_cmds::AddFromResumeDataCmd;
  using SaveAllResumeDataCmd = session_cmds::SaveAllResumeDataCmd;
  using PauseCmd = session_cmds::PauseCmd;
  using ResumeCmd = session_cmds::ResumeCmd;
  using RemoveCmd = session_cmds::RemoveCmd;
  using MoveStorageCmd = session_cmds::MoveStorageCmd;
  using EditTrackersCmd = session_cmds::EditTrackersCmd;
  using ForceRecheckCmd = session_cmds::ForceRecheckCmd;
  using ForceReannounceCmd = session_cmds::ForceReannounceCmd;
  using SetSequentialDownloadCmd = session_cmds::SetSequentialDownloadCmd;
  using SetAutoManagedCmd = session_cmds::SetAutoManagedCmd;
  using ForceStartCmd = session_cmds::ForceStartCmd;
  using SetFirstLastPiecePriorityCmd = session_cmds::SetFirstLastPiecePriorityCmd;
  using QueuePositionUpCmd = session_cmds::QueuePositionUpCmd;
  using QueuePositionDownCmd = session_cmds::QueuePositionDownCmd;
  using QueuePositionTopCmd = session_cmds::QueuePositionTopCmd;
  using QueuePositionBottomCmd = session_cmds::QueuePositionBottomCmd;
  using ApplyRuntimeSettingsCmd = session_cmds::ApplyRuntimeSettingsCmd;
  using SetDefaultPerTorrentConnectionsLimitCmd =
      session_cmds::SetDefaultPerTorrentConnectionsLimitCmd;
  using SetTaskConnectionsLimitCmd = session_cmds::SetTaskConnectionsLimitCmd;
  using QueryTaskCopyPayloadCmd = session_cmds::QueryTaskCopyPayloadCmd;
  using QuerySessionStatsCmd = session_cmds::QuerySessionStatsCmd;
  using QueryTaskFilesCmd = session_cmds::QueryTaskFilesCmd;
  using QueryTaskTrackersCmd = session_cmds::QueryTaskTrackersCmd;
  using QueryTaskPeersCmd = session_cmds::QueryTaskPeersCmd;
  using QueryTaskWebSeedsCmd = session_cmds::QueryTaskWebSeedsCmd;
  using SetTaskFilePriorityCmd = session_cmds::SetTaskFilePriorityCmd;
  using RenameTaskFileOrFolderCmd = session_cmds::RenameTaskFileOrFolderCmd;
  using AddTaskTrackerCmd = session_cmds::AddTaskTrackerCmd;
  using EditTaskTrackerCmd = session_cmds::EditTaskTrackerCmd;
  using RemoveTaskTrackerCmd = session_cmds::RemoveTaskTrackerCmd;
  using ForceReannounceTrackerCmd = session_cmds::ForceReannounceTrackerCmd;
  using ForceReannounceAllTrackersCmd = session_cmds::ForceReannounceAllTrackersCmd;
  using ShutdownCmd = session_cmds::ShutdownCmd;
  using Cmd = session_cmds::Cmd;

  std::mutex mu;
  std::condition_variable cv;
  std::deque<Cmd> cmds;
  AlertsCallback alertsCb;
  std::thread worker;
  bool running{true};
  std::map<QString, libtorrent::torrent_handle> handlesByTaskId;
  int defaultPerTorrentConnectionsLimit{0};
  int defaultPerTorrentUploadSlotsLimit{0};
  std::map<QString, int> perTaskConnectionsLimit;
  std::map<QString, SessionWorker::AddTorrentOptions> pendingAddOpts;
  std::map<QString, AddTorrentTorrentInfoConstPtr> preparedTorrentInfo;
  std::map<QString, std::shared_ptr<std::promise<std::optional<SessionWorker::MagnetMetadata>>>>
      pendingMagnetMeta;
  int pendingResumeDataSaves{0};
  int completedResumeDataSaves{0};
  QString resumeDataDir;
  std::shared_ptr<std::promise<int>> resumeDataDone;
  std::shared_ptr<std::promise<SessionWorker::SessionStats>> pendingSessionStats_;

  void enqueue(Cmd cmd) {
    const auto describeCmd = [](const Cmd& c) -> const char* {
      if (std::holds_alternative<AddMagnetCmd>(c)) {
        return "AddMagnet";
      }
      if (std::holds_alternative<AddTorrentFileCmd>(c)) {
        return "AddTorrentFile";
      }
      if (std::holds_alternative<PauseCmd>(c)) {
        return "Pause";
      }
      if (std::holds_alternative<ResumeCmd>(c)) {
        return "Resume";
      }
      if (std::holds_alternative<RemoveCmd>(c)) {
        return "Remove";
      }
      if (std::holds_alternative<MoveStorageCmd>(c)) {
        return "MoveStorage";
      }
      if (std::holds_alternative<EditTrackersCmd>(c)) {
        return "EditTrackers";
      }
      if (std::holds_alternative<ForceRecheckCmd>(c)) {
        return "ForceRecheck";
      }
      if (std::holds_alternative<ForceReannounceCmd>(c)) {
        return "ForceReannounce";
      }
      if (std::holds_alternative<SetSequentialDownloadCmd>(c)) {
        return "SetSequentialDownload";
      }
      if (std::holds_alternative<SetAutoManagedCmd>(c)) {
        return "SetAutoManaged";
      }
      if (std::holds_alternative<ForceStartCmd>(c)) {
        return "ForceStart";
      }
      if (std::holds_alternative<SetFirstLastPiecePriorityCmd>(c)) {
        return "SetFirstLastPiecePriority";
      }
      if (std::holds_alternative<QueuePositionUpCmd>(c)) {
        return "QueuePositionUp";
      }
      if (std::holds_alternative<QueuePositionDownCmd>(c)) {
        return "QueuePositionDown";
      }
      if (std::holds_alternative<QueuePositionTopCmd>(c)) {
        return "QueuePositionTop";
      }
      if (std::holds_alternative<QueuePositionBottomCmd>(c)) {
        return "QueuePositionBottom";
      }
      if (std::holds_alternative<ApplyRuntimeSettingsCmd>(c)) {
        return "ApplyRuntimeSettings";
      }
      if (std::holds_alternative<SetDefaultPerTorrentConnectionsLimitCmd>(c)) {
        return "SetDefaultPerTorrentConnectionsLimit";
      }
      if (std::holds_alternative<SetTaskConnectionsLimitCmd>(c)) {
        return "SetTaskConnectionsLimit";
      }
      if (std::holds_alternative<QueryTaskCopyPayloadCmd>(c)) {
        return "QueryTaskCopyPayload";
      }
      if (std::holds_alternative<QuerySessionStatsCmd>(c)) {
        return "QuerySessionStats";
      }
      if (std::holds_alternative<QueryTaskFilesCmd>(c)) {
        return "QueryTaskFiles";
      }
      if (std::holds_alternative<QueryTaskTrackersCmd>(c)) {
        return "QueryTaskTrackers";
      }
      if (std::holds_alternative<QueryTaskPeersCmd>(c)) {
        return "QueryTaskPeers";
      }
      if (std::holds_alternative<QueryTaskWebSeedsCmd>(c)) {
        return "QueryTaskWebSeeds";
      }
      if (std::holds_alternative<SetTaskFilePriorityCmd>(c)) {
        return "SetTaskFilePriority";
      }
      if (std::holds_alternative<RenameTaskFileOrFolderCmd>(c)) {
        return "RenameTaskFileOrFolder";
      }
      if (std::holds_alternative<AddTaskTrackerCmd>(c)) {
        return "AddTaskTracker";
      }
      if (std::holds_alternative<EditTaskTrackerCmd>(c)) {
        return "EditTaskTracker";
      }
      if (std::holds_alternative<RemoveTaskTrackerCmd>(c)) {
        return "RemoveTaskTracker";
      }
      if (std::holds_alternative<ForceReannounceTrackerCmd>(c)) {
        return "ForceReannounceTracker";
      }
      if (std::holds_alternative<ForceReannounceAllTrackersCmd>(c)) {
        return "ForceReannounceAllTrackers";
      }
      if (std::holds_alternative<PrepareMagnetMetadataCmd>(c)) {
        return "PrepareMagnetMetadata";
      }
      if (std::holds_alternative<CancelPreparedMagnetCmd>(c)) {
        return "CancelPreparedMagnet";
      }
      if (std::holds_alternative<FinalizePreparedMagnetCmd>(c)) {
        return "FinalizePreparedMagnet";
      }
      if (std::holds_alternative<AddFromResumeDataCmd>(c)) {
        return "AddFromResumeData";
      }
      if (std::holds_alternative<SaveAllResumeDataCmd>(c)) {
        return "SaveAllResumeData";
      }
      return "Shutdown";
    };
    {
      std::lock_guard<std::mutex> lk(mu);
      const auto queueSize = cmds.size() + 1;
      LOG_DEBUG(QStringLiteral("[lt.worker] Enqueue cmd=%1 queue=%2")
                    .arg(QString::fromLatin1(describeCmd(cmd)))
                    .arg(queueSize));
      cmds.push_back(std::move(cmd));
    }
    cv.notify_one();
  }

  void run() {
    libtorrent::settings_pack sp;
    sp.set_int(libtorrent::settings_pack::alert_mask,
               libtorrent::alert_category::error | libtorrent::alert_category::status |
                   libtorrent::alert_category::storage | libtorrent::alert_category::tracker |
                   libtorrent::alert_category::dht | libtorrent::alert_category::peer_log);
    sp.set_bool(libtorrent::settings_pack::enable_dht, true);
    sp.set_bool(libtorrent::settings_pack::enable_lsd, true);
    sp.set_bool(libtorrent::settings_pack::enable_upnp, true);
    sp.set_bool(libtorrent::settings_pack::enable_natpmp, true);

    libtorrent::session ses(sp);
    LOG_INFO(QStringLiteral("[lt.worker] Session thread started."));

    while (running) {
      std::deque<Cmd> local;
      {
        std::unique_lock<std::mutex> lk(mu);
        cv.wait_for(lk, std::chrono::milliseconds(200),
                    [this] { return !cmds.empty() || !running; });
        local.swap(cmds);
      }

      std::vector<LtAlertView> syntheticFromCmds;
      for (auto& cmd : local) {
        session_ops::Context ctx{handlesByTaskId,         defaultPerTorrentConnectionsLimit,
                                 perTaskConnectionsLimit, pendingAddOpts,
                                 preparedTorrentInfo,     pendingMagnetMeta,
                                 pendingResumeDataSaves,  completedResumeDataSaves,
                                 resumeDataDir,           resumeDataDone,
                                 &syntheticFromCmds,      &pendingSessionStats_};
        if (session_ops::handleCmd(ses, ctx, cmd)) {
          continue;
        }
        if (std::holds_alternative<ShutdownCmd>(cmd)) {
          running = false;
          break;
        }
      }

      if (pendingSessionStats_) {
        ses.post_session_stats();
      }

      ses.post_torrent_updates();
      std::vector<libtorrent::alert*> alerts;
      ses.pop_alerts(&alerts);
      std::vector<LtAlertView> views = std::move(syntheticFromCmds);
      for (const libtorrent::alert* a : alerts) {
        if (auto* stats_alert = libtorrent::alert_cast<libtorrent::session_stats_alert>(a);
            stats_alert != nullptr && pendingSessionStats_) {
          SessionWorker::SessionStats s{};
          const int dht_idx = libtorrent::find_metric_idx("dht.dht_nodes");
          const auto vals = stats_alert->counters();
          if (dht_idx >= 0 &&
              static_cast<std::size_t>(dht_idx) < static_cast<std::size_t>(vals.size())) {
            s.dht_nodes = static_cast<int>(vals[static_cast<std::size_t>(dht_idx)]);
          }
          for (auto const& th : ses.get_torrents()) {
            if (!th.is_valid()) {
              continue;
            }
            const auto ts = th.status();
            s.download_rate += static_cast<qint64>(ts.download_rate);
            s.upload_rate += static_cast<qint64>(ts.upload_rate);
          }
          pendingSessionStats_->set_value(s);
          pendingSessionStats_.reset();
          continue;
        }
        if (auto* meta = libtorrent::alert_cast<libtorrent::metadata_received_alert>(a);
            meta != nullptr) {
          const auto ih = meta->handle.info_hashes();
          const auto id = session_ids::taskIdFromInfoHashes(ih);
          const QString key = session_ids::taskIdKey(id);
          auto itPromise = pendingMagnetMeta.find(key);
          if (itPromise != pendingMagnetMeta.end() && itPromise->second != nullptr) {
            SessionWorker::MagnetMetadata m;
            m.taskId = id;
            if (auto ti = meta->handle.torrent_file_with_hashes(); ti) {
              preparedTorrentInfo[key] = ti;
              m.name = QString::fromStdString(ti->name());
              m.comment = QString::fromStdString(ti->comment());
              m.creationDate = static_cast<qint64>(ti->creation_date());
              m.totalBytes = 0;
              const auto& fs = ti->files();
              const int n = fs.num_files();
              m.filePaths.reserve(static_cast<size_t>(n));
              m.fileSizes.reserve(static_cast<size_t>(n));
              for (int i = 0; i < n; ++i) {
                const auto idx = libtorrent::file_index_t{i};
                const qint64 sz = static_cast<qint64>(fs.file_size(idx));
                m.totalBytes += sz;
                m.filePaths.push_back(QString::fromStdString(fs.file_path(idx)));
                m.fileSizes.push_back(sz);
              }
              if (ti->info_hashes().has_v1()) {
                std::ostringstream oss;
                oss << ti->info_hashes().v1;
                m.infoHashV1 = QString::fromStdString(oss.str());
              }
              if (ti->info_hashes().has_v2()) {
                std::ostringstream oss;
                oss << ti->info_hashes().v2;
                m.infoHashV2 = QString::fromStdString(oss.str());
              }
            }
            itPromise->second->set_value(std::make_optional(std::move(m)));
            pendingMagnetMeta.erase(itPromise);
          }
        }

        // --- resume data handling ---
        if (auto* rd = libtorrent::alert_cast<libtorrent::save_resume_data_alert>(a);
            rd != nullptr) {
          if (!resumeDataDir.isEmpty()) {
            const auto id = session_ids::taskIdFromInfoHashes(rd->params.info_hashes);
            const QString key = session_ids::taskIdKey(id);
            const auto buf = libtorrent::write_resume_data_buf(rd->params);
            const QString path = QDir(resumeDataDir).filePath(key + QStringLiteral(".rd"));
            QFile f(path);
            if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
              f.write(reinterpret_cast<const char*>(buf.data()), static_cast<qint64>(buf.size()));
              f.close();
              ++completedResumeDataSaves;
            }
          }
          --pendingResumeDataSaves;
          if (pendingResumeDataSaves <= 0 && resumeDataDone) {
            resumeDataDone->set_value(completedResumeDataSaves);
            resumeDataDone.reset();
          }
          continue;
        }
        if (libtorrent::alert_cast<libtorrent::save_resume_data_failed_alert>(a) != nullptr) {
          --pendingResumeDataSaves;
          if (pendingResumeDataSaves <= 0 && resumeDataDone) {
            resumeDataDone->set_value(completedResumeDataSaves);
            resumeDataDone.reset();
          }
          continue;
        }

        if (auto* add = libtorrent::alert_cast<libtorrent::add_torrent_alert>(a); add != nullptr) {
          if (!add->error) {
            const auto id = session_ids::taskIdFromInfoHashes(add->params.info_hashes);
            const QString key = session_ids::taskIdKey(id);
            handlesByTaskId[key] = add->handle;
            add->handle.resume();
            const auto itOpts = pendingAddOpts.find(key);
            if (itOpts != pendingAddOpts.end()) {
              if (itOpts->second.add_to_top_queue) {
                add->handle.queue_position_top();
              }
              pendingAddOpts.erase(itOpts);
            }
            const auto itOverride = perTaskConnectionsLimit.find(key);
            if (itOverride != perTaskConnectionsLimit.end()) {
              add->handle.set_max_connections(itOverride->second);
            } else if (defaultPerTorrentConnectionsLimit > 0) {
              add->handle.set_max_connections(defaultPerTorrentConnectionsLimit);
            }
            if (defaultPerTorrentUploadSlotsLimit > 0) {
              add->handle.set_max_uploads(defaultPerTorrentUploadSlotsLimit);
            }
          }
        } else if (auto* removed = libtorrent::alert_cast<libtorrent::torrent_removed_alert>(a);
                   removed != nullptr) {
          const auto id = session_ids::taskIdFromInfoHashes(removed->info_hashes);
          const QString key = session_ids::taskIdKey(id);
          handlesByTaskId.erase(key);
          perTaskConnectionsLimit.erase(key);
          pendingAddOpts.erase(key);
        }

        auto v = adaptAlertToViews(a);
        for (auto& view : v) {
          if (view.type == LtAlertView::Type::kTaskAdded) {
            auto hIt = handlesByTaskId.find(session_ids::taskIdKey(view.taskId));
            if (hIt != handlesByTaskId.end() && hIt->second.is_valid()) {
              view.magnet = QString::fromStdString(libtorrent::make_magnet_uri(hIt->second));
            }
          }
        }
        views.insert(views.end(), v.begin(), v.end());
      }
      AlertsCallback cb;
      {
        std::lock_guard<std::mutex> lk(mu);
        cb = alertsCb;
      }
      if (cb && !views.empty()) {
        LOG_DEBUG(QStringLiteral("[lt.worker] Dispatch alerts views=%1").arg(views.size()));
        cb(std::move(views));
      }
    }
    LOG_INFO(QStringLiteral("[lt.worker] Session thread stopped."));
  }
};

SessionWorker::SessionWorker() : impl_(new Impl()) {
  impl_->worker = std::thread([this] { impl_->run(); });
}

SessionWorker::~SessionWorker() {
  impl_->enqueue(Impl::ShutdownCmd{});
  if (impl_->worker.joinable()) {
    impl_->worker.join();
  }
  delete impl_;
}

void SessionWorker::setAlertsCallback(AlertsCallback cb) {
  std::lock_guard<std::mutex> lk(impl_->mu);
  impl_->alertsCb = std::move(cb);
}

void SessionWorker::addMagnet(const QString& uri, const QString& savePath,
                              const QStringList& trackers) {
  impl_->enqueue(Impl::AddMagnetCmd{uri, savePath, trackers});
}

std::optional<SessionWorker::MagnetMetadata>
SessionWorker::prepareMagnetMetadata(const QString& uri, const QString& tempSavePath,
                                     int timeout_ms) {
  libtorrent::error_code ec;
  libtorrent::add_torrent_params atp =
      libtorrent::parse_magnet_uri(uri.trimmed().toStdString(), ec);
  if (ec) {
    return std::nullopt;
  }
  const auto taskId = session_ids::taskIdFromInfoHashes(atp.info_hashes);

  auto done = std::make_shared<std::promise<std::optional<SessionWorker::MagnetMetadata>>>();
  auto fut = done->get_future();
  Impl::PrepareMagnetMetadataCmd cmd;
  cmd.uri = uri;
  cmd.tempSavePath = tempSavePath;
  cmd.timeout_ms = timeout_ms;
  cmd.done = done;
  impl_->enqueue(std::move(cmd));
  if (fut.wait_for(std::chrono::milliseconds(timeout_ms)) == std::future_status::ready) {
    return fut.get();
  }
  cancelPreparedMagnet(taskId);
  return std::nullopt;
}

void SessionWorker::cancelPreparedMagnet(const pfd::base::TaskId& taskId) {
  impl_->enqueue(Impl::CancelPreparedMagnetCmd{taskId});
}

void SessionWorker::finalizePreparedMagnet(const pfd::base::TaskId& taskId, const QString& savePath,
                                           const QStringList& trackers,
                                           const SessionWorker::AddTorrentOptions& opts) {
  Impl::FinalizePreparedMagnetCmd cmd;
  cmd.taskId = taskId;
  cmd.savePath = savePath;
  cmd.trackers = trackers;
  cmd.opts = opts;
  impl_->enqueue(std::move(cmd));
}

void SessionWorker::addFromResumeData(const QByteArray& resumeData, bool startPaused) {
  impl_->enqueue(Impl::AddFromResumeDataCmd{resumeData, startPaused});
}

int SessionWorker::saveAllResumeData(const QString& resumeDir) {
  QDir().mkpath(resumeDir);
  auto done = std::make_shared<std::promise<int>>();
  auto fut = done->get_future();
  impl_->enqueue(Impl::SaveAllResumeDataCmd{resumeDir, done});
  if (fut.wait_for(std::chrono::seconds(15)) == std::future_status::ready) {
    return fut.get();
  }
  return 0;
}

void SessionWorker::addTorrentFile(const QString& filePath, const QString& savePath,
                                   const QStringList& trackers) {
  Impl::AddTorrentFileCmd cmd;
  cmd.filePath = filePath;
  cmd.savePath = savePath;
  cmd.trackers = trackers;
  cmd.has_opts = false;
  impl_->enqueue(std::move(cmd));
}

void SessionWorker::addTorrentFileWithOptions(const QString& filePath, const QString& savePath,
                                              const QStringList& trackers,
                                              const SessionWorker::AddTorrentOptions& opts) {
  Impl::AddTorrentFileCmd cmd;
  cmd.filePath = filePath;
  cmd.savePath = savePath;
  cmd.trackers = trackers;
  cmd.opts = opts;
  cmd.has_opts = true;
  impl_->enqueue(std::move(cmd));
}

void SessionWorker::pauseTask(const pfd::base::TaskId& taskId) {
  impl_->enqueue(Impl::PauseCmd{taskId});
}

void SessionWorker::resumeTask(const pfd::base::TaskId& taskId) {
  impl_->enqueue(Impl::ResumeCmd{taskId});
}

void SessionWorker::removeTask(const pfd::base::TaskId& taskId, bool removeFiles) {
  impl_->enqueue(Impl::RemoveCmd{taskId, removeFiles});
}

void SessionWorker::moveStorage(const pfd::base::TaskId& taskId, const QString& targetPath) {
  impl_->enqueue(Impl::MoveStorageCmd{taskId, targetPath});
}

void SessionWorker::editTrackers(const pfd::base::TaskId& taskId, const QStringList& trackers) {
  impl_->enqueue(Impl::EditTrackersCmd{taskId, trackers});
}

void SessionWorker::forceRecheckTask(const pfd::base::TaskId& taskId) {
  impl_->enqueue(Impl::ForceRecheckCmd{taskId});
}

void SessionWorker::forceReannounceTask(const pfd::base::TaskId& taskId) {
  impl_->enqueue(Impl::ForceReannounceCmd{taskId});
}

void SessionWorker::setSequentialDownload(const pfd::base::TaskId& taskId, bool enabled) {
  impl_->enqueue(Impl::SetSequentialDownloadCmd{taskId, enabled});
}

void SessionWorker::setAutoManagedTask(const pfd::base::TaskId& taskId, bool enabled) {
  impl_->enqueue(Impl::SetAutoManagedCmd{taskId, enabled});
}

void SessionWorker::forceStartTask(const pfd::base::TaskId& taskId) {
  impl_->enqueue(Impl::ForceStartCmd{taskId});
}

void SessionWorker::setFirstLastPiecePriority(const pfd::base::TaskId& taskId, bool enabled) {
  impl_->enqueue(Impl::SetFirstLastPiecePriorityCmd{taskId, enabled});
}

void SessionWorker::queuePositionUp(const pfd::base::TaskId& taskId) {
  impl_->enqueue(Impl::QueuePositionUpCmd{taskId});
}

void SessionWorker::queuePositionDown(const pfd::base::TaskId& taskId) {
  impl_->enqueue(Impl::QueuePositionDownCmd{taskId});
}

void SessionWorker::queuePositionTop(const pfd::base::TaskId& taskId) {
  impl_->enqueue(Impl::QueuePositionTopCmd{taskId});
}

void SessionWorker::queuePositionBottom(const pfd::base::TaskId& taskId) {
  impl_->enqueue(Impl::QueuePositionBottomCmd{taskId});
}

void SessionWorker::applyRuntimeSettings(
    int download_rate_limit_kib, int upload_rate_limit_kib, int connections_limit,
    int upload_slots_limit, int per_torrent_upload_slots_limit, int listen_port,
    bool listen_port_forwarding, int active_downloads, int active_seeds, int active_limit,
    int max_active_checking, bool auto_manage_prefer_seeds, bool dont_count_slow_torrents,
    int slow_torrent_dl_rate_threshold, int slow_torrent_ul_rate_threshold,
    int slow_torrent_inactive_timer, bool enable_dht, bool enable_upnp, bool enable_natpmp,
    bool enable_lsd, bool proxy_enabled, const QString& proxy_type, const QString& proxy_host,
    int proxy_port, const QString& proxy_username, const QString& proxy_password,
    const QString& encryption_mode) {
  Impl::ApplyRuntimeSettingsCmd cmd;
  cmd.download_rate_limit_kib = download_rate_limit_kib;
  cmd.upload_rate_limit_kib = upload_rate_limit_kib;
  cmd.connections_limit = connections_limit;
  cmd.upload_slots_limit = upload_slots_limit;
  cmd.per_torrent_upload_slots_limit = per_torrent_upload_slots_limit;
  cmd.listen_port = listen_port;
  cmd.listen_port_forwarding = listen_port_forwarding;
  cmd.active_downloads = active_downloads;
  cmd.active_seeds = active_seeds;
  cmd.active_limit = active_limit;
  cmd.max_active_checking = max_active_checking;
  cmd.auto_manage_prefer_seeds = auto_manage_prefer_seeds;
  cmd.dont_count_slow_torrents = dont_count_slow_torrents;
  cmd.slow_torrent_dl_rate_threshold = slow_torrent_dl_rate_threshold;
  cmd.slow_torrent_ul_rate_threshold = slow_torrent_ul_rate_threshold;
  cmd.slow_torrent_inactive_timer = slow_torrent_inactive_timer;
  cmd.enable_dht = enable_dht;
  cmd.enable_upnp = enable_upnp;
  cmd.enable_natpmp = enable_natpmp;
  cmd.enable_lsd = enable_lsd;
  cmd.proxy_enabled = proxy_enabled;
  cmd.proxy_type = proxy_type;
  cmd.proxy_host = proxy_host;
  cmd.proxy_port = proxy_port;
  cmd.proxy_username = proxy_username;
  cmd.proxy_password = proxy_password;
  cmd.encryption_mode = encryption_mode;
  {
    std::lock_guard<std::mutex> lk(impl_->mu);
    impl_->defaultPerTorrentUploadSlotsLimit = std::clamp(per_torrent_upload_slots_limit, 0, 20000);
  }
  impl_->enqueue(std::move(cmd));
}

void SessionWorker::setDefaultPerTorrentConnectionsLimit(int per_torrent_connections_limit) {
  impl_->enqueue(Impl::SetDefaultPerTorrentConnectionsLimitCmd{per_torrent_connections_limit});
}

void SessionWorker::setTaskConnectionsLimit(const pfd::base::TaskId& taskId, int limit) {
  impl_->enqueue(Impl::SetTaskConnectionsLimitCmd{taskId, limit});
}

SessionWorker::CopyPayload SessionWorker::queryTaskCopyPayload(const pfd::base::TaskId& taskId) {
  auto done = std::make_shared<std::promise<SessionWorker::CopyPayload>>();
  auto fut = done->get_future();
  impl_->enqueue(Impl::QueryTaskCopyPayloadCmd{taskId, done});
  if (fut.wait_for(std::chrono::milliseconds(1200)) == std::future_status::ready) {
    return fut.get();
  }
  SessionWorker::CopyPayload out;
  out.torrentId = taskId.toString(QUuid::WithoutBraces);
  return out;
}

SessionWorker::SessionStats SessionWorker::querySessionStats() {
  auto done = std::make_shared<std::promise<SessionWorker::SessionStats>>();
  auto fut = done->get_future();
  impl_->enqueue(Impl::QuerySessionStatsCmd{done});
  if (fut.wait_for(std::chrono::milliseconds(1200)) == std::future_status::ready) {
    return fut.get();
  }
  return {};
}

SessionWorker::TaskTrackerSnapshot
SessionWorker::queryTaskTrackers(const pfd::base::TaskId& taskId) {
  auto done = std::make_shared<std::promise<SessionWorker::TaskTrackerSnapshot>>();
  auto fut = done->get_future();
  impl_->enqueue(Impl::QueryTaskTrackersCmd{taskId, done});
  if (fut.wait_for(std::chrono::milliseconds(1200)) == std::future_status::ready) {
    return fut.get();
  }
  return {};
}

std::vector<SessionWorker::FileEntrySnapshot>
SessionWorker::queryTaskFiles(const pfd::base::TaskId& taskId) {
  auto done = std::make_shared<std::promise<std::vector<SessionWorker::FileEntrySnapshot>>>();
  auto fut = done->get_future();
  impl_->enqueue(Impl::QueryTaskFilesCmd{taskId, done});
  if (fut.wait_for(std::chrono::milliseconds(1200)) == std::future_status::ready) {
    return fut.get();
  }
  return {};
}

std::vector<SessionWorker::PeerSnapshot>
SessionWorker::queryTaskPeers(const pfd::base::TaskId& taskId) {
  auto done = std::make_shared<std::promise<std::vector<SessionWorker::PeerSnapshot>>>();
  auto fut = done->get_future();
  impl_->enqueue(Impl::QueryTaskPeersCmd{taskId, done});
  if (fut.wait_for(std::chrono::milliseconds(1200)) == std::future_status::ready) {
    return fut.get();
  }
  return {};
}

std::vector<SessionWorker::WebSeedSnapshot>
SessionWorker::queryTaskWebSeeds(const pfd::base::TaskId& taskId) {
  auto done = std::make_shared<std::promise<std::vector<SessionWorker::WebSeedSnapshot>>>();
  auto fut = done->get_future();
  impl_->enqueue(Impl::QueryTaskWebSeedsCmd{taskId, done});
  if (fut.wait_for(std::chrono::milliseconds(1200)) == std::future_status::ready) {
    return fut.get();
  }
  return {};
}

void SessionWorker::setTaskFilePriority(const pfd::base::TaskId& taskId,
                                        const std::vector<int>& fileIndices,
                                        FilePriorityLevel level) {
  impl_->enqueue(Impl::SetTaskFilePriorityCmd{taskId, fileIndices, level});
}

void SessionWorker::renameTaskFileOrFolder(const pfd::base::TaskId& taskId,
                                           const QString& logicalPath, const QString& newName) {
  impl_->enqueue(Impl::RenameTaskFileOrFolderCmd{taskId, logicalPath, newName});
}

void SessionWorker::addTaskTracker(const pfd::base::TaskId& taskId, const QString& url) {
  impl_->enqueue(Impl::AddTaskTrackerCmd{taskId, url});
}

void SessionWorker::editTaskTracker(const pfd::base::TaskId& taskId, const QString& oldUrl,
                                    const QString& newUrl) {
  impl_->enqueue(Impl::EditTaskTrackerCmd{taskId, oldUrl, newUrl});
}

void SessionWorker::removeTaskTracker(const pfd::base::TaskId& taskId, const QString& url) {
  impl_->enqueue(Impl::RemoveTaskTrackerCmd{taskId, url});
}

void SessionWorker::forceReannounceTracker(const pfd::base::TaskId& taskId, const QString& url) {
  impl_->enqueue(Impl::ForceReannounceTrackerCmd{taskId, url});
}

void SessionWorker::forceReannounceAllTrackers(const pfd::base::TaskId& taskId) {
  impl_->enqueue(Impl::ForceReannounceAllTrackersCmd{taskId});
}

}  // namespace pfd::lt
