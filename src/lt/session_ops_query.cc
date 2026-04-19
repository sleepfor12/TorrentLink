#include <QtCore/QUrl>
#include <QtCore/QUuid>

#include <libtorrent/announce_entry.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/peer_info.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_flags.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>

#include <algorithm>
#include <chrono>
#include <sstream>

#include "core/logger.h"
#include "lt/session_ids.h"
#include "lt/session_ops.h"

namespace pfd::lt::session_ops {

namespace {

SessionWorker::FilePriorityLevel toLevel(libtorrent::download_priority_t p, bool sequential) {
  const int v = static_cast<int>(static_cast<std::uint8_t>(p));
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

// announce_infohash: see libtorrent/announce_entry.hpp — "working" requires a successful
// response (start/scrape), not merely fails==0 before the first attempt.
[[nodiscard]] static int secondsUntilTimePoint32(libtorrent::time_point32 tp) {
  if (tp == (libtorrent::time_point32::min)()) {
    return -1;
  }
  const auto now =
      std::chrono::time_point_cast<libtorrent::seconds32>(libtorrent::clock_type::now());
  if (tp <= now) {
    return 0;
  }
  return static_cast<int>((tp - now).count());
}

[[nodiscard]] static bool infohashHasSuccessfulContact(const libtorrent::announce_infohash& ih) {
  return ih.start_sent || ih.complete_sent || ih.scrape_incomplete >= 0 ||
         ih.scrape_complete >= 0 || ih.scrape_downloaded >= 0;
}

[[nodiscard]] static SessionWorker::TrackerStatus
mapInfohashStatus(const libtorrent::announce_infohash& ih) {
  using T = SessionWorker::TrackerStatus;
  if (ih.updating) {
    return T::kUpdating;
  }
  if (ih.fails > 0 || static_cast<bool>(ih.last_error)) {
    return T::kCannotConnect;
  }
  if (infohashHasSuccessfulContact(ih)) {
    return T::kWorking;
  }
  return T::kNotWorking;
}

[[nodiscard]] static SessionWorker::TrackerStatus
aggregateInfohashStatuses(const libtorrent::announce_endpoint& ep) {
  using T = SessionWorker::TrackerStatus;
  bool anyUpdating = false;
  bool anyWorking = false;
  bool anyCannot = false;
  for (const auto& ih : ep.info_hashes) {
    const T s = mapInfohashStatus(ih);
    if (s == T::kUpdating) {
      anyUpdating = true;
    } else if (s == T::kWorking) {
      anyWorking = true;
    } else if (s == T::kCannotConnect) {
      anyCannot = true;
    }
  }
  if (anyUpdating) {
    return T::kUpdating;
  }
  if (anyWorking) {
    return T::kWorking;
  }
  if (anyCannot) {
    return T::kCannotConnect;
  }
  return T::kNotWorking;
}

[[nodiscard]] static SessionWorker::TrackerStatus
mapTrackerStatus(const libtorrent::announce_entry& ae) {
  using T = SessionWorker::TrackerStatus;
  bool anyUpdating = false;
  bool anyWorking = false;
  bool anyCannot = false;
  for (const auto& ep : ae.endpoints) {
    for (const auto& ih : ep.info_hashes) {
      const T s = mapInfohashStatus(ih);
      if (s == T::kUpdating) {
        anyUpdating = true;
      } else if (s == T::kWorking) {
        anyWorking = true;
      } else if (s == T::kCannotConnect) {
        anyCannot = true;
      }
    }
  }
  if (anyUpdating) {
    return T::kUpdating;
  }
  if (anyWorking) {
    return T::kWorking;
  }
  if (anyCannot) {
    return T::kCannotConnect;
  }
  return T::kNotWorking;
}

[[nodiscard]] static QString messageFromAnnounceEndpoint(const libtorrent::announce_endpoint& ep) {
  QStringList parts;
  for (const auto& ih : ep.info_hashes) {
    if (!ih.message.empty()) {
      parts.append(QString::fromStdString(ih.message));
    } else if (ih.last_error) {
      parts.append(QString::fromStdString(ih.last_error.message()));
    }
  }
  if (parts.isEmpty()) {
    return {};
  }
  return parts.join(QStringLiteral("; "));
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

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::QuerySessionStatsCmd& c) {
  if (c.done == nullptr) {
    return true;
  }
  if (ctx.pendingSessionStats != nullptr) {
    *ctx.pendingSessionStats = c.done;
    return true;
  }
  SessionWorker::SessionStats out{};
  c.done->set_value(out);
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
      const bool sequential = bool(status.flags & libtorrent::torrent_flags::sequential_download);
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
          s.progress01 = std::clamp(
              static_cast<double>(s.downloadedBytes) / static_cast<double>(s.sizeBytes), 0.0, 1.0);
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
    const bool dhtEnabled = !bool(st.flags & libtorrent::torrent_flags::disable_dht);
    const bool pexEnabled = !bool(st.flags & libtorrent::torrent_flags::disable_pex);
    const bool lsdEnabled = !bool(st.flags & libtorrent::torrent_flags::disable_lsd);
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
      if (low.startsWith(QStringLiteral("udp://")))
        row.btProtocol = QStringLiteral("UDP");
      else if (low.startsWith(QStringLiteral("http://")) ||
               low.startsWith(QStringLiteral("https://")))
        row.btProtocol = QStringLiteral("HTTP");
      else if (low.startsWith(QStringLiteral("wss://")))
        row.btProtocol = QStringLiteral("WSS");
      else
        row.btProtocol = QStringLiteral("N/A");
      row.status = mapTrackerStatus(ae);

      QStringList rowMsgParts;
      for (const auto& ep : ae.endpoints) {
        int ep_scrape_inc = -1;
        int ep_scrape_comp = -1;
        int ep_scrape_down = -1;
        for (const auto& ih : ep.info_hashes) {
          if (ep_scrape_inc < 0 && ih.scrape_incomplete >= 0) {
            ep_scrape_inc = ih.scrape_incomplete;
          }
          if (ep_scrape_comp < 0 && ih.scrape_complete >= 0) {
            ep_scrape_comp = ih.scrape_complete;
          }
          if (ep_scrape_down < 0 && ih.scrape_downloaded >= 0) {
            ep_scrape_down = ih.scrape_downloaded;
          }
        }
        if (row.users < 0 && ep_scrape_inc >= 0) {
          row.users = ep_scrape_inc;
        }
        if (row.seeds < 0 && ep_scrape_comp >= 0) {
          row.seeds = ep_scrape_comp;
        }
        if (row.downloads < 0 && ep_scrape_down >= 0) {
          row.downloads = ep_scrape_down;
        }
        SessionWorker::TrackerEndpointSnapshot outEp;
        outEp.ip = QString::fromStdString(ep.local_endpoint.address().to_string());
        outEp.port = ep.local_endpoint.port();
        outEp.status = aggregateInfohashStatuses(ep);
        outEp.users = ep_scrape_inc;
        outEp.seeds = ep_scrape_comp;
        outEp.downloads = ep_scrape_down;
        const bool hasV1 = h.info_hashes().has_v1();
        const bool hasV2 = h.info_hashes().has_v2();
        if (hasV1 && hasV2) {
          outEp.btProtocol = QStringLiteral("hybrid");
        } else if (hasV2) {
          outEp.btProtocol = QStringLiteral("v2");
        } else {
          outEp.btProtocol = QStringLiteral("v1");
        }
        int epNextSec = -1;
        int epMinSec = -1;
        for (const auto& ih : ep.info_hashes) {
          const int n = secondsUntilTimePoint32(ih.next_announce);
          const int m = secondsUntilTimePoint32(ih.min_announce);
          if (n >= 0) {
            epNextSec = epNextSec < 0 ? n : std::min(epNextSec, n);
          }
          if (m >= 0) {
            epMinSec = epMinSec < 0 ? m : std::min(epMinSec, m);
          }
        }
        outEp.nextAnnounceSec = epNextSec;
        outEp.minAnnounceSec = epMinSec;
        if (epNextSec >= 0) {
          if (row.nextAnnounceSec < 0) {
            row.nextAnnounceSec = epNextSec;
          } else {
            row.nextAnnounceSec = std::min(row.nextAnnounceSec, epNextSec);
          }
        }
        if (epMinSec >= 0) {
          if (row.minAnnounceSec < 0) {
            row.minAnnounceSec = epMinSec;
          } else {
            row.minAnnounceSec = std::min(row.minAnnounceSec, epMinSec);
          }
        }
        outEp.message = messageFromAnnounceEndpoint(ep);
        if (!outEp.message.isEmpty()) {
          rowMsgParts.append(outEp.message);
        }
        if (!outEp.ip.isEmpty() && outEp.port > 0) {
          row.endpoints.push_back(std::move(outEp));
        }
      }
      row.message = rowMsgParts.isEmpty() ? QString() : rowMsgParts.join(QStringLiteral(" | "));
      if (row.endpoints.empty()) {
        const QUrl u(row.url);
        if (u.isValid() && !u.host().trimmed().isEmpty() && u.port() > 0) {
          SessionWorker::TrackerEndpointSnapshot endpoint;
          endpoint.ip = u.host().trimmed();
          endpoint.port = u.port();
          const bool hasV1 = h.info_hashes().has_v1();
          const bool hasV2 = h.info_hashes().has_v2();
          if (hasV1 && hasV2)
            endpoint.btProtocol = QStringLiteral("hybrid");
          else if (hasV2)
            endpoint.btProtocol = QStringLiteral("v2");
          else
            endpoint.btProtocol = QStringLiteral("v1");
          endpoint.status = row.status;
          endpoint.users = row.users;
          endpoint.seeds = row.seeds;
          endpoint.downloads = row.downloads;
          endpoint.nextAnnounceSec = row.nextAnnounceSec;
          endpoint.minAnnounceSec = row.minAnnounceSec;
          endpoint.message = row.message;
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
      if (p.flags & libtorrent::peer_info::seed)
        flags += QStringLiteral("S");
      if (p.flags & libtorrent::peer_info::interesting)
        flags += QStringLiteral("I");
      if (p.flags & libtorrent::peer_info::choked)
        flags += QStringLiteral("C");
      if (p.flags & libtorrent::peer_info::remote_interested)
        flags += QStringLiteral("i");
      if (p.flags & libtorrent::peer_info::remote_choked)
        flags += QStringLiteral("c");
      if (p.flags & libtorrent::peer_info::rc4_encrypted)
        flags += QStringLiteral("E");
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
