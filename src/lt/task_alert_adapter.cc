#include "lt/task_alert_adapter.h"

#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_info.hpp>

#include <chrono>
#include <sstream>

#include "base/time_stamps.h"
#include "lt/session_ids.h"

namespace pfd::lt {

namespace {
QString toHexV1(const libtorrent::info_hash_t& ih) {
  if (!ih.has_v1())
    return {};
  std::ostringstream oss;
  oss << ih.v1;
  return QString::fromStdString(oss.str());
}

QString toHexV2(const libtorrent::info_hash_t& ih) {
  if (!ih.has_v2())
    return {};
  std::ostringstream oss;
  oss << ih.v2;
  return QString::fromStdString(oss.str());
}

pfd::base::TaskStatus mapTorrentState(const libtorrent::torrent_status& status) {
  using S = libtorrent::torrent_status::state_t;
  if (bool(status.flags & libtorrent::torrent_flags::paused)) {
    return pfd::base::TaskStatus::kPaused;
  }
  switch (status.state) {
    case S::checking_files:
    case S::checking_resume_data:
      return pfd::base::TaskStatus::kChecking;
    case S::downloading_metadata:
    case S::downloading:
      return pfd::base::TaskStatus::kDownloading;
    case S::finished:
      return pfd::base::TaskStatus::kFinished;
    case S::seeding:
      return pfd::base::TaskStatus::kSeeding;
    default:
      return pfd::base::TaskStatus::kQueued;
  }
}

}  // namespace

std::optional<LtAlertView> adaptAlertToView(const libtorrent::alert* alert) {
  if (alert == nullptr) {
    return std::nullopt;
  }

  LtAlertView v;

  if (auto* add = libtorrent::alert_cast<libtorrent::add_torrent_alert>(alert); add != nullptr) {
    v.taskId = session_ids::taskIdFromInfoHashes(add->params.info_hashes);
    if (add->error) {
      v.type = LtAlertView::Type::kTaskError;
      v.message = QString::fromStdString(add->error.message());
    } else {
      v.type = LtAlertView::Type::kTaskAdded;
      v.name = QString::fromStdString(add->params.name);
      v.savePath = QString::fromStdString(add->params.save_path);
      v.infoHashV1 = toHexV1(add->params.info_hashes);
      v.infoHashV2 = toHexV2(add->params.info_hashes);
      if (add->params.ti) {
        v.comment = QString::fromStdString(add->params.ti->comment());
        v.pieces = add->params.ti->num_pieces();
        v.pieceLength = add->params.ti->piece_length();
        v.createdBy = QString::fromStdString(add->params.ti->creator());
        const auto cd = add->params.ti->creation_date();
        if (cd > 0) {
          v.createdOnMs = static_cast<qint64>(cd) * 1000;
        }
        v.isPrivate = add->params.ti->priv() ? 1 : 0;
        v.totalBytes = add->params.ti->total_size();
      }
      v.category = QStringLiteral("Default");
      v.status = pfd::base::TaskStatus::kQueued;
    }
    return v;
  }

  if (auto* err = libtorrent::alert_cast<libtorrent::torrent_error_alert>(alert); err != nullptr) {
    v.taskId = session_ids::taskIdFromInfoHashes(err->handle.info_hashes());
    v.type = LtAlertView::Type::kTaskError;
    v.message = QString::fromStdString(err->error.message());
    return v;
  }

  if (auto* removed = libtorrent::alert_cast<libtorrent::torrent_removed_alert>(alert);
      removed != nullptr) {
    v.taskId = session_ids::taskIdFromInfoHashes(removed->info_hashes);
    v.type = LtAlertView::Type::kTaskRemoved;
    return v;
  }

  return std::nullopt;
}

LtAlertView adaptTorrentStatusToProgressView(const libtorrent::torrent_status& status) {
  LtAlertView v;
  v.taskId = session_ids::taskIdFromInfoHashes(status.info_hashes);
  v.type = LtAlertView::Type::kTaskProgress;
  v.name = QString::fromStdString(status.name);
  v.totalBytes = status.total_wanted;
  v.selectedBytes = status.total_wanted;
  v.downloadedBytes = status.total_done;
  v.uploadedBytes = status.total_upload;
  v.downloadRate = status.download_rate;
  v.uploadRate = status.upload_rate;
  v.seeders = status.num_seeds;
  v.users = status.num_peers;
  v.availability = status.distributed_copies;
  v.infoHashV1 = toHexV1(status.info_hashes);
  v.infoHashV2 = toHexV2(status.info_hashes);
  v.category = QStringLiteral("Default");
  v.status = mapTorrentState(status);

  v.activeTimeSec =
      std::chrono::duration_cast<std::chrono::seconds>(status.active_duration).count();
  v.wastedBytes = status.total_failed_bytes + status.total_redundant_bytes;

  const auto nextAnn = status.next_announce;
  v.nextAnnounceSec = std::chrono::duration_cast<std::chrono::seconds>(nextAnn).count();

  const auto lsc = status.last_seen_complete;
  if (lsc > 0) {
    v.lastSeenCompleteMs = static_cast<qint64>(lsc) * 1000;
  }

  if (status.has_metadata) {
    const auto ti = status.torrent_file.lock();
    if (ti) {
      v.pieces = ti->num_pieces();
      v.pieceLength = ti->piece_length();
      v.createdBy = QString::fromStdString(ti->creator());
      const auto cd = ti->creation_date();
      if (cd > 0) {
        v.createdOnMs = static_cast<qint64>(cd) * 1000;
      }
      v.isPrivate = ti->priv() ? 1 : 0;
    }
  }

  if (status.completed_time > 0) {
    v.completedOnMs = static_cast<qint64>(status.completed_time) * 1000;
  }

  return v;
}

std::vector<LtAlertView> adaptAlertToViews(const libtorrent::alert* alert) {
  std::vector<LtAlertView> out;
  if (alert == nullptr) {
    return out;
  }

  if (auto* su = libtorrent::alert_cast<libtorrent::state_update_alert>(alert); su != nullptr) {
    out.reserve(su->status.size());
    for (const auto& st : su->status) {
      out.push_back(adaptTorrentStatusToProgressView(st));
    }
    return out;
  }

  if (const auto one = adaptAlertToView(alert); one.has_value()) {
    out.push_back(*one);
  }
  return out;
}

}  // namespace pfd::lt
