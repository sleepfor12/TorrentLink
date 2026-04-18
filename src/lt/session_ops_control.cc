#include <libtorrent/announce_entry.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_flags.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>

#include "core/logger.h"
#include "lt/session_ids.h"
#include "lt/session_ops.h"

namespace pfd::lt::session_ops {

namespace {

libtorrent::download_priority_t toLtPriority(SessionWorker::FilePriorityLevel level) {
  switch (level) {
    case SessionWorker::FilePriorityLevel::kDoNotDownload:
      return libtorrent::download_priority_t{0};
    case SessionWorker::FilePriorityLevel::kNotDownloaded:
      return libtorrent::download_priority_t{1};
    case SessionWorker::FilePriorityLevel::kHigh:
      return libtorrent::download_priority_t{6};
    case SessionWorker::FilePriorityLevel::kHighest:
      return libtorrent::download_priority_t{7};
    case SessionWorker::FilePriorityLevel::kFileOrder:
    case SessionWorker::FilePriorityLevel::kNormal:
    default:
      return libtorrent::download_priority_t{4};
  }
}

std::vector<libtorrent::announce_entry>
mergedTrackers(const std::vector<libtorrent::announce_entry>& oldList, const QStringList& urls) {
  std::vector<libtorrent::announce_entry> out;
  out.reserve(urls.size());
  int tier = 0;
  for (const auto& u : urls) {
    const QString t = u.trimmed();
    if (t.isEmpty())
      continue;
    libtorrent::announce_entry ae(t.toStdString());
    ae.tier = tier++;
    const auto it = std::find_if(oldList.begin(), oldList.end(), [&](const auto& old) {
      return QString::fromStdString(old.url).trimmed() == t;
    });
    if (it != oldList.end()) {
      ae.source = it->source;
      ae.fail_limit = it->fail_limit;
    }
    out.push_back(std::move(ae));
  }
  return out;
}

}  // namespace

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::PauseCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end()) {
    it->second.unset_flags(libtorrent::torrent_flags::auto_managed);
    it->second.pause();
    LOG_INFO(QStringLiteral("[lt.worker] Pause applied taskId=%1").arg(c.taskId.toString()));
  } else {
    LOG_WARN(QStringLiteral("[lt.worker] Pause ignored, handle not found taskId=%1")
                 .arg(c.taskId.toString()));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::ResumeCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end()) {
    it->second.set_flags(libtorrent::torrent_flags::auto_managed);
    it->second.resume();
    LOG_INFO(QStringLiteral("[lt.worker] Resume applied taskId=%1").arg(c.taskId.toString()));
  } else {
    LOG_WARN(QStringLiteral("[lt.worker] Resume ignored, handle not found taskId=%1")
                 .arg(c.taskId.toString()));
  }
  return true;
}

bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::RemoveCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end()) {
    if (c.removeFiles) {
      ses.remove_torrent(it->second, libtorrent::session::delete_files);
    } else {
      ses.remove_torrent(it->second);
    }
    LOG_INFO(QStringLiteral("[lt.worker] Remove applied taskId=%1").arg(c.taskId.toString()));
  } else {
    LOG_WARN(QStringLiteral("[lt.worker] Remove ignored, handle not found taskId=%1")
                 .arg(c.taskId.toString()));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::MoveStorageCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end()) {
    it->second.move_storage(c.targetPath.toStdString());
    LOG_INFO(QStringLiteral("[lt.worker] Move storage requested taskId=%1 target=%2")
                 .arg(c.taskId.toString(), c.targetPath));
  } else {
    LOG_WARN(QStringLiteral("[lt.worker] Move storage ignored, handle not found taskId=%1")
                 .arg(c.taskId.toString()));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::EditTrackersCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end()) {
    std::vector<libtorrent::announce_entry> entries;
    entries.reserve(c.trackers.size());
    for (const auto& tracker : c.trackers) {
      const QString t = tracker.trimmed();
      if (!t.isEmpty()) {
        entries.emplace_back(t.toStdString());
      }
    }
    it->second.replace_trackers(entries);
    LOG_INFO(QStringLiteral("[lt.worker] Trackers updated taskId=%1 count=%2")
                 .arg(c.taskId.toString())
                 .arg(entries.size()));
  } else {
    LOG_WARN(QStringLiteral("[lt.worker] Edit trackers ignored, handle not found taskId=%1")
                 .arg(c.taskId.toString()));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::ForceRecheckCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    it->second.force_recheck();
    LOG_INFO(
        QStringLiteral("[lt.worker] Force recheck requested taskId=%1").arg(c.taskId.toString()));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::ForceReannounceCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    it->second.force_reannounce();
    LOG_INFO(QStringLiteral("[lt.worker] Force reannounce requested taskId=%1")
                 .arg(c.taskId.toString()));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx,
               const session_cmds::SetSequentialDownloadCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    if (c.enabled) {
      it->second.set_flags(libtorrent::torrent_flags::sequential_download);
    } else {
      it->second.unset_flags(libtorrent::torrent_flags::sequential_download);
    }
    LOG_INFO(QStringLiteral("[lt.worker] Sequential download taskId=%1 enabled=%2")
                 .arg(c.taskId.toString())
                 .arg(c.enabled ? QStringLiteral("true") : QStringLiteral("false")));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::SetAutoManagedCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    if (c.enabled) {
      it->second.set_flags(libtorrent::torrent_flags::auto_managed);
    } else {
      it->second.unset_flags(libtorrent::torrent_flags::auto_managed);
    }
    LOG_INFO(QStringLiteral("[lt.worker] Auto managed taskId=%1 enabled=%2")
                 .arg(c.taskId.toString())
                 .arg(c.enabled ? QStringLiteral("true") : QStringLiteral("false")));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::ForceStartCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    it->second.unset_flags(libtorrent::torrent_flags::auto_managed);
    it->second.resume();
    LOG_INFO(QStringLiteral("[lt.worker] Force start taskId=%1").arg(c.taskId.toString()));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx,
               const session_cmds::SetFirstLastPiecePriorityCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it == ctx.handlesByTaskId.end() || !it->second.is_valid()) {
    return true;
  }
  auto& h = it->second;
  auto ti = h.torrent_file();
  if (!ti) {
    LOG_WARN(QStringLiteral("[lt.worker] FirstLastPiece ignored, no torrent_info taskId=%1")
                 .arg(c.taskId.toString()));
    return true;
  }
  const int numPieces = ti->num_pieces();
  if (numPieces == 0) {
    return true;
  }
  const auto prio = c.enabled ? libtorrent::download_priority_t{static_cast<std::uint8_t>(7)}
                              : libtorrent::download_priority_t{static_cast<std::uint8_t>(4)};
  const auto& fs = ti->files();
  for (int fi = 0; fi < fs.num_files(); ++fi) {
    const auto fidx = libtorrent::file_index_t{fi};
    const auto pieceRange = ti->map_file(fidx, 0, 1);
    const int firstPiece = static_cast<int>(pieceRange.piece);
    const auto endRange = ti->map_file(fidx, std::max<std::int64_t>(fs.file_size(fidx) - 1, 0), 1);
    const int lastPiece = static_cast<int>(endRange.piece);
    h.piece_priority(libtorrent::piece_index_t{firstPiece}, prio);
    if (lastPiece != firstPiece) {
      h.piece_priority(libtorrent::piece_index_t{lastPiece}, prio);
    }
  }
  LOG_INFO(QStringLiteral("[lt.worker] FirstLastPiece taskId=%1 enabled=%2")
               .arg(c.taskId.toString())
               .arg(c.enabled ? QStringLiteral("true") : QStringLiteral("false")));
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::QueuePositionUpCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    it->second.queue_position_up();
    LOG_INFO(QStringLiteral("[lt.worker] Queue position up taskId=%1").arg(c.taskId.toString()));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::QueuePositionDownCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    it->second.queue_position_down();
    LOG_INFO(QStringLiteral("[lt.worker] Queue position down taskId=%1").arg(c.taskId.toString()));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::QueuePositionTopCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    it->second.queue_position_top();
    LOG_INFO(QStringLiteral("[lt.worker] Queue position top taskId=%1").arg(c.taskId.toString()));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::QueuePositionBottomCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    it->second.queue_position_bottom();
    LOG_INFO(
        QStringLiteral("[lt.worker] Queue position bottom taskId=%1").arg(c.taskId.toString()));
  }
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::SaveAllResumeDataCmd& c) {
  ctx.resumeDataDir = c.resumeDir;
  ctx.resumeDataDone = c.done;
  ctx.pendingResumeDataSaves = 0;
  ctx.completedResumeDataSaves = 0;
  for (auto& [key, h] : ctx.handlesByTaskId) {
    if (h.is_valid()) {
      h.save_resume_data(libtorrent::torrent_handle::save_info_dict);
      ++ctx.pendingResumeDataSaves;
    }
  }
  if (ctx.pendingResumeDataSaves == 0 && ctx.resumeDataDone) {
    ctx.resumeDataDone->set_value(0);
    ctx.resumeDataDone.reset();
  }
  LOG_INFO(QStringLiteral("[lt.worker] SaveAllResumeData requested count=%1")
               .arg(ctx.pendingResumeDataSaves));
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::SetTaskFilePriorityCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it == ctx.handlesByTaskId.end() || !it->second.is_valid()) {
    LOG_WARN(QStringLiteral("[lt.worker] SetTaskFilePriority ignored, handle not found taskId=%1")
                 .arg(c.taskId.toString()));
    return true;
  }
  auto h = it->second;
  auto priorities = h.get_file_priorities();
  if (priorities.empty()) {
    LOG_WARN(QStringLiteral("[lt.worker] SetTaskFilePriority ignored, no priorities taskId=%1")
                 .arg(c.taskId.toString()));
    return true;
  }
  if (c.level == SessionWorker::FilePriorityLevel::kFileOrder) {
    h.set_flags(libtorrent::torrent_flags::sequential_download);
  }
  const auto target = toLtPriority(c.level);
  for (const int raw : c.fileIndices) {
    if (raw < 0 || raw >= static_cast<int>(priorities.size())) {
      continue;
    }
    priorities[static_cast<size_t>(raw)] = target;
  }
  h.prioritize_files(priorities);
  LOG_INFO(QStringLiteral("[lt.worker] SetTaskFilePriority applied taskId=%1 level=%2 files=%3")
               .arg(c.taskId.toString())
               .arg(static_cast<int>(c.level))
               .arg(c.fileIndices.size()));
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::AddTaskTrackerCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it == ctx.handlesByTaskId.end() || !it->second.is_valid()) {
    LOG_WARN(QStringLiteral("[lt.worker] AddTaskTracker ignored, handle not found taskId=%1 url=%2")
                 .arg(c.taskId.toString(), c.url));
    return true;
  }
  auto h = it->second;
  const auto old = h.trackers();
  QStringList urls;
  urls.reserve(static_cast<int>(old.size()) + 1);
  for (const auto& ae : old)
    urls.push_back(QString::fromStdString(ae.url));
  urls.push_back(c.url);
  h.replace_trackers(mergedTrackers(old, urls));
  LOG_INFO(QStringLiteral("[lt.worker] AddTaskTracker applied taskId=%1 url=%2")
               .arg(c.taskId.toString(), c.url));
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::EditTaskTrackerCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it == ctx.handlesByTaskId.end() || !it->second.is_valid()) {
    LOG_WARN(QStringLiteral(
                 "[lt.worker] EditTaskTracker ignored, handle not found taskId=%1 old=%2 new=%3")
                 .arg(c.taskId.toString(), c.oldUrl, c.newUrl));
    return true;
  }
  auto h = it->second;
  const auto old = h.trackers();
  QStringList urls;
  urls.reserve(static_cast<int>(old.size()));
  for (const auto& ae : old) {
    const QString u = QString::fromStdString(ae.url);
    if (u.trimmed() == c.oldUrl.trimmed())
      urls.push_back(c.newUrl.trimmed());
    else
      urls.push_back(u);
  }
  h.replace_trackers(mergedTrackers(old, urls));
  LOG_INFO(QStringLiteral("[lt.worker] EditTaskTracker applied taskId=%1 old=%2 new=%3")
               .arg(c.taskId.toString(), c.oldUrl, c.newUrl));
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx, const session_cmds::RemoveTaskTrackerCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it == ctx.handlesByTaskId.end() || !it->second.is_valid()) {
    LOG_WARN(
        QStringLiteral("[lt.worker] RemoveTaskTracker ignored, handle not found taskId=%1 url=%2")
            .arg(c.taskId.toString(), c.url));
    return true;
  }
  auto h = it->second;
  const auto old = h.trackers();
  QStringList urls;
  urls.reserve(static_cast<int>(old.size()));
  for (const auto& ae : old) {
    const QString u = QString::fromStdString(ae.url);
    if (u.trimmed() != c.url.trimmed())
      urls.push_back(u);
  }
  h.replace_trackers(mergedTrackers(old, urls));
  LOG_INFO(QStringLiteral("[lt.worker] RemoveTaskTracker applied taskId=%1 url=%2")
               .arg(c.taskId.toString(), c.url));
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx,
               const session_cmds::ForceReannounceTrackerCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it == ctx.handlesByTaskId.end() || !it->second.is_valid())
    return true;
  auto h = it->second;
  const auto trackers = h.trackers();
  for (int i = 0; i < static_cast<int>(trackers.size()); ++i) {
    const QString u = QString::fromStdString(trackers[static_cast<size_t>(i)].url).trimmed();
    if (u == c.url.trimmed()) {
      h.force_reannounce(0, i);
      LOG_INFO(QStringLiteral("[lt.worker] Force reannounce tracker taskId=%1 url=%2")
                   .arg(c.taskId.toString(), c.url));
      return true;
    }
  }
  h.force_reannounce();
  LOG_WARN(
      QStringLiteral("[lt.worker] Tracker not found for reannounce, fallback all taskId=%1 url=%2")
          .arg(c.taskId.toString(), c.url));
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx,
               const session_cmds::ForceReannounceAllTrackersCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it == ctx.handlesByTaskId.end() || !it->second.is_valid())
    return true;
  it->second.force_reannounce();
  LOG_INFO(QStringLiteral("[lt.worker] Force reannounce all trackers taskId=%1")
               .arg(c.taskId.toString()));
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx,
               const session_cmds::RenameTaskFileOrFolderCmd& c) {
  const auto it = ctx.handlesByTaskId.find(session_ids::taskIdKey(c.taskId));
  if (it == ctx.handlesByTaskId.end() || !it->second.is_valid()) {
    LOG_WARN(
        QStringLiteral("[lt.worker] RenameTaskFileOrFolder ignored, handle not found taskId=%1")
            .arg(c.taskId.toString()));
    return true;
  }
  auto h = it->second;
  auto ti = h.torrent_file();
  if (!ti) {
    LOG_WARN(QStringLiteral("[lt.worker] RenameTaskFileOrFolder ignored, no torrent_info taskId=%1")
                 .arg(c.taskId.toString()));
    return true;
  }
  const QString src = c.logicalPath.trimmed();
  const QString targetName = c.newName.trimmed();
  if (src.isEmpty() || targetName.isEmpty()) {
    LOG_WARN(
        QStringLiteral(
            "[lt.worker] RenameTaskFileOrFolder ignored, invalid args taskId=%1 path=%2 newName=%3")
            .arg(c.taskId.toString(), c.logicalPath, c.newName));
    return true;
  }
  const int slash = src.lastIndexOf('/');
  const QString parent = slash >= 0 ? src.left(slash) : QString();
  const QString renamedRoot =
      parent.isEmpty() ? targetName : (parent + QStringLiteral("/") + targetName);
  const auto& fs = ti->files();
  for (int i = 0; i < fs.num_files(); ++i) {
    const auto idx = libtorrent::file_index_t{i};
    const QString p = QString::fromStdString(fs.file_path(idx));
    if (p == src) {
      h.rename_file(idx, renamedRoot.toStdString());
      continue;
    }
    const QString folderPrefix = src + QStringLiteral("/");
    if (p.startsWith(folderPrefix)) {
      const QString suffix = p.mid(folderPrefix.size());
      h.rename_file(idx, (renamedRoot + QStringLiteral("/") + suffix).toStdString());
    }
  }
  LOG_INFO(QStringLiteral("[lt.worker] RenameTaskFileOrFolder applied taskId=%1 from=%2 to=%3")
               .arg(c.taskId.toString(), src, renamedRoot));
  return true;
}

}  // namespace pfd::lt::session_ops
