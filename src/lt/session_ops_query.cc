#include <QtCore/QUuid>
#include <QtCore/QUrl>

#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/peer_info.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/session_status.hpp>
#include <libtorrent/announce_entry.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>

#include <sstream>
#include <algorithm>

#include "core/logger.h"
#include "lt/session_ids.h"
#include "lt/session_ops.h"

namespace pfd::lt::session_ops {

namespace {

SessionWorker::FilePriorityLevel toLevel(libtorrent::download_priority_t p, bool sequential) {
  const int v = static_cast<int>(p);
  if (v <= 0) {
    return SessionWorker::FilePriorityLevel::kDoNotDownload;
  }
  if (sequential) {
    return SessionWorker::FilePriorityLevel::kFileOrder;
  }
  if (v >= 7) {
    return SessionWorker::FilePriorityLevel::kHighest;
  }
  if (v >= 6) {
    return SessionWorker::FilePriorityLevel::kHigh;
  }
  if (v >= 4) {
    return SessionWorker::FilePriorityLevel::kNormal;
  }
  return SessionWorker::FilePriorityLevel::kNotDownloaded;
}

}  // namespace

static SessionWorker::TrackerStatus mapTrackerStatus(const libtorrent::announce_entry& ae) {
  bool updating = false;
  bool hasError = false;
  bool worked = false;
  for (const auto& ep : ae.endpoints) {
    updating = updating || ep.updating;
    hasError = hasError || static_cast<bool>(ep.last_error) || ep.fails > 0;
    worked = worked || ep.is_working();
  }
  if (updating) return SessionWorker::TrackerStatus::kUpdating;
  if (worked) return SessionWorker::TrackerStatus::kWorking;
  if (hasError) return SessionWorker::TrackerStatus::kCannotConnect;
  return SessionWorker::TrackerStatus::kNotWorking;
}

static SessionWorker::TrackerStatus mapEndpointStatus(const libtorrent::announce_endpoint& ep) {
  bool updating = false;
  bool hasError = false;
  bool worked = false;
  for (const auto& ih : ep.info_hashes) {
    updating = updating || ih.updating;
    hasError = hasError || static_cast<bool>(ih.last_error) || ih.fails > 0;
    worked = worked || (ih.fails == 0 && !ih.updating);
  }
  if (updating) return SessionWorker::TrackerStatus::kUpdating;
  if (worked) return SessionWorker::TrackerStatus::kWorking;
  if (hasError) return SessionWorker::TrackerStatus::kCannotConnect;
  return SessionWorker::TrackerStatus::kNotWorking;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::QueryTaskCopyPayloadCmd& c) {
  SessionWorker::CopyPayload payload;
  payload.torrentId = c.taskId.toString(QUuid::WithoutBraces);
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    const auto ih = it->second.info_hashes();
    if (ih.has_v1()) {
      std::ostringstream oss;
      oss << ih.v1;
      payload.infoHashV1 = QString::fromStdString(oss.str());
    }
    if (ih.has_v2()) {
      std::ostringstream oss;
      oss << ih.v2;
      payload.infoHashV2 = QString::fromStdString(oss.str());
    }
    payload.magnet = QString::fromStdString(libtorrent::make_magnet_uri(it->second));
    if (auto ti = it->second.torrent_file(); ti) {
      payload.comment = QString::fromStdString(ti->comment());
    }
  }
  if (c.done != nullptr) {
    c.done->set_value(std::move(payload));
  }
  return true;
}

bool handleOne(libtorrent::session& ses, Context&, const session_cmds::QuerySessionStatsCmd& c) {
  SessionWorker::SessionStats out;
  const auto ss = ses.status();
  out.dht_nodes = ss.dht_nodes;
  out.download_rate = static_cast<qint64>(ss.download_rate);
  out.upload_rate = static_cast<qint64>(ss.upload_rate);
  if (c.done != nullptr) {
    c.done->set_value(out);
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::QueryTaskFilesCmd& c) {
  std::vector<SessionWorker::FileEntrySnapshot> out;
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    auto h = it->second;
    auto ti = h.torrent_file();
    if (ti) {
      const auto status = h.status();
      const bool sequential = status.sequential_download;
      const double fileAvailability = status.distributed_copies;
      const auto fileProgress = h.file_progress();
      const auto priorities = h.get_file_priorities();
      const auto& fs = ti->files();
      out.reserve(static_cast<size_t>(fs.num_files()));
      for (int i = 0; i < fs.num_files(); ++i) {
        const auto idx = libtorrent::file_index_t{i};
        SessionWorker::FileEntrySnapshot s;
        s.fileIndex = i;
        s.logicalPath = QString::fromStdString(fs.file_path(idx));
        s.sizeBytes = static_cast<qint64>(fs.file_size(idx));
        if (i < static_cast<int>(fileProgress.size())) {
          s.downloadedBytes = static_cast<qint64>(fileProgress[static_cast<size_t>(i)]);
        }
        if (s.sizeBytes > 0) {
          s.progress01 = std::clamp(static_cast<double>(s.downloadedBytes) /
                                        static_cast<double>(s.sizeBytes),
                                    0.0, 1.0);
        }
        s.availability = fileAvailability;
        if (i < static_cast<int>(priorities.size())) {
          s.priority = toLevel(priorities[static_cast<size_t>(i)], sequential);
        }
        out.push_back(std::move(s));
      }
      LOG_DEBUG(QStringLiteral("[lt.worker] Query task files taskId=%1 files=%2")
                    .arg(c.taskId.toString())
                    .arg(out.size()));
    }
  } else {
    LOG_WARN(QStringLiteral("[lt.worker] Query task files ignored, handle not found taskId=%1")
                 .arg(c.taskId.toString()));
  }
  if (c.done != nullptr) {
    c.done->set_value(std::move(out));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::QueryTaskTrackersCmd& c) {
  SessionWorker::TaskTrackerSnapshot out;
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    auto h = it->second;
    const auto st = h.status();
    const bool dhtEnabled = (st.flags & libtorrent::torrent_flags::disable_dht) == 0;
    const bool pexEnabled = (st.flags & libtorrent::torrent_flags::disable_pex) == 0;
    const bool lsdEnabled = (st.flags & libtorrent::torrent_flags::disable_lsd) == 0;
    auto makeFixed = [](const QString& name, bool enabled) {
      SessionWorker::TrackerRowSnapshot r;
      r.url = name;
      r.readOnly = true;
      r.btProtocol = QStringLiteral("N/A");
      r.status = enabled ? SessionWorker::TrackerStatus::kWorking
                         : SessionWorker::TrackerStatus::kNotWorking;
      return r;
    };
    out.fixedRows.push_back(makeFixed(QStringLiteral("[DHT]"), dhtEnabled));
    out.fixedRows.push_back(makeFixed(QStringLiteral("[PeX]"), pexEnabled));
    out.fixedRows.push_back(makeFixed(QStringLiteral("[LSD]"), lsdEnabled));

    const auto trackers = h.trackers();
    out.trackers.reserve(trackers.size());
    for (const auto& ae : trackers) {
      SessionWorker::TrackerRowSnapshot row;
      row.url = QString::fromStdString(ae.url);
      row.tier = ae.tier;
      const QString low = row.url.toLower();
      if (low.startsWith(QStringLiteral("udp://"))) row.btProtocol = QStringLiteral("UDP");
      else if (low.startsWith(QStringLiteral("http://")) || low.startsWith(QStringLiteral("https://"))) row.btProtocol = QStringLiteral("HTTP");
      else if (low.startsWith(QStringLiteral("wss://"))) row.btProtocol = QStringLiteral("WSS");
      else row.btProtocol = QStringLiteral("N/A");
      row.status = mapTrackerStatus(ae);

      for (const auto& ep : ae.endpoints) {
        if (row.users < 0 && ep.scrape_incomplete >= 0) row.users = ep.scrape_incomplete;
        if (row.seeds < 0 && ep.scrape_complete >= 0) row.seeds = ep.scrape_complete;
        if (row.downloads < 0 && ep.scrape_downloaded >= 0) row.downloads = ep.scrape_downloaded;
        SessionWorker::TrackerEndpointSnapshot endpoint;
        endpoint.ip = QString::fromStdString(ep.local_endpoint.address().to_string());
        endpoint.port = ep.local_endpoint.port();
        endpoint.status = mapEndpointStatus(ep);
        endpoint.users = ep.scrape_incomplete;
        endpoint.seeds = ep.scrape_complete;
        endpoint.downloads = ep.scrape_downloaded;
        const bool hasV1 = h.info_hashes().has_v1();
        const bool hasV2 = h.info_hashes().has_v2();
        if (hasV1 && hasV2) endpoint.btProtocol = QStringLiteral("hybrid");
        else if (hasV2) endpoint.btProtocol = QStringLiteral("v2");
        else endpoint.btProtocol = QStringLiteral("v1");
        if (!endpoint.ip.isEmpty() && endpoint.port > 0) row.endpoints.push_back(endpoint);
        if (row.nextAnnounceSec < 0 && ep.next_announce != (libtorrent::time_point32::min)()) {
          row.nextAnnounceSec = 0;
        }
        if (row.minAnnounceSec < 0 && ep.min_announce != (libtorrent::time_point32::min)()) {
          row.minAnnounceSec = 0;
        }
      }
      if (row.endpoints.empty()) {
        const QUrl u(row.url);
        if (u.isValid() && !u.host().trimmed().isEmpty() && u.port() > 0) {
          SessionWorker::TrackerEndpointSnapshot endpoint;
          endpoint.ip = u.host().trimmed();
          endpoint.port = u.port();
          const bool hasV1 = h.info_hashes().has_v1();
          const bool hasV2 = h.info_hashes().has_v2();
          if (hasV1 && hasV2) endpoint.btProtocol = QStringLiteral("hybrid");
          else if (hasV2) endpoint.btProtocol = QStringLiteral("v2");
          else endpoint.btProtocol = QStringLiteral("v1");
          endpoint.status = row.status;
          endpoint.users = row.users;
          endpoint.seeds = row.seeds;
          endpoint.downloads = row.downloads;
          endpoint.nextAnnounceSec = row.nextAnnounceSec;
          endpoint.minAnnounceSec = row.minAnnounceSec;
          row.endpoints.push_back(std::move(endpoint));
        }
      }
      out.trackers.push_back(std::move(row));
    }
    LOG_DEBUG(QStringLiteral("[lt.worker] Query trackers taskId=%1 fixed=%2 trackers=%3")
                  .arg(c.taskId.toString())
                  .arg(out.fixedRows.size())
                  .arg(out.trackers.size()));
  } else {
    LOG_WARN(QStringLiteral("[lt.worker] Query trackers ignored, handle not found taskId=%1")
                 .arg(c.taskId.toString()));
  }
  if (c.done != nullptr) {
    c.done->set_value(std::move(out));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::QueryTaskPeersCmd& c) {
  std::vector<SessionWorker::PeerSnapshot> out;
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    std::vector<libtorrent::peer_info> peers;
    it->second.get_peer_info(peers);
    out.reserve(peers.size());
    for (const auto& p : peers) {
      SessionWorker::PeerSnapshot s;
      s.ip = QString::fromStdString(p.ip.address().to_string());
      s.port = p.ip.port();
      s.client = QString::fromStdString(p.client);
      s.progress = p.progress;
      s.downloadRate = p.payload_down_speed;
      s.uploadRate = p.payload_up_speed;
      s.totalDownloaded = p.total_download;
      s.totalUploaded = p.total_upload;
      QString flags;
      if (p.flags & libtorrent::peer_info::seed) flags += QStringLiteral("S");
      if (p.flags & libtorrent::peer_info::interesting) flags += QStringLiteral("I");
      if (p.flags & libtorrent::peer_info::choked) flags += QStringLiteral("C");
      if (p.flags & libtorrent::peer_info::remote_interested) flags += QStringLiteral("i");
      if (p.flags & libtorrent::peer_info::remote_choked) flags += QStringLiteral("c");
      if (p.flags & libtorrent::peer_info::rc4_encrypted) flags += QStringLiteral("E");
      s.flags = flags;
      if (p.connection_type == libtorrent::peer_info::standard_bittorrent)
        s.connection = QStringLiteral("BT");
      else if (p.connection_type == libtorrent::peer_info::web_seed)
        s.connection = QStringLiteral("Web");
      else if (p.connection_type == libtorrent::peer_info::http_seed)
        s.connection = QStringLiteral("HTTP");
      else
        s.connection = QStringLiteral("?");
      out.push_back(std::move(s));
    }
  }
  if (c.done != nullptr) {
    c.done->set_value(std::move(out));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::QueryTaskWebSeedsCmd& c) {
  std::vector<SessionWorker::WebSeedSnapshot> out;
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    const auto urlSeeds = it->second.url_seeds();
    for (const auto& u : urlSeeds) {
      out.push_back({QString::fromStdString(u), QStringLiteral("BEP19")});
    }
    const auto httpSeeds = it->second.http_seeds();
    for (const auto& u : httpSeeds) {
      out.push_back({QString::fromStdString(u), QStringLiteral("BEP17")});
    }
  }
  if (c.done != nullptr) {
    c.done->set_value(std::move(out));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context&, const session_cmds::ShutdownCmd&) {
  return false;
}

}  // namespace pfd::lt::session_ops
