#include <QtCore/QDateTime>

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/download_priority.hpp>
#include <libtorrent/info_hash.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_flags.hpp>
#include <libtorrent/torrent_info.hpp>

#include <algorithm>

#include "base/input_sanitizer.h"
#include "core/logger.h"
#include "lt/add_torrent_torrent_info_ptr.h"
#include "lt/session_ids.h"
#include "lt/session_ops.h"

namespace pfd::lt::session_ops {

namespace {

bool hasActiveTaskForInfoHashes(const Context& ctx, const libtorrent::info_hash_t& ih) {
  const QString key = session_ids::taskIdKey(session_ids::taskIdFromInfoHashes(ih));
  const auto it = ctx.handlesByTaskId.find(key);
  return it != ctx.handlesByTaskId.end() && it->second.is_valid();
}

void emitDuplicateRejected(Context& ctx, const libtorrent::info_hash_t& ih) {
  if (ctx.syntheticAlertViews == nullptr) {
    return;
  }
  LtAlertView v;
  v.type = LtAlertView::Type::kDuplicateRejected;
  v.taskId = session_ids::taskIdFromInfoHashes(ih);
  v.message = QStringLiteral("任务已存在（相同 info-hash），已忽略重复添加");
  v.eventMs = QDateTime::currentMSecsSinceEpoch();
  ctx.syntheticAlertViews->push_back(std::move(v));
}

}  // namespace

bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::AddMagnetCmd& c) {
  const QString trimmed = c.uri.trimmed();
  const auto valErr = pfd::base::validateMagnetUri(trimmed);
  if (valErr.hasError()) {
    LOG_ERROR(QStringLiteral("SessionWorker addMagnet rejected: %1").arg(valErr.message()));
    return true;
  }
  libtorrent::error_code ec;
  libtorrent::add_torrent_params atp = libtorrent::parse_magnet_uri(trimmed.toStdString(), ec);
  if (ec) {
    LOG_ERROR(QString("SessionWorker parse_magnet_uri failed: %1")
                  .arg(QString::fromStdString(ec.message())));
    return true;
  }
  if (hasActiveTaskForInfoHashes(ctx, atp.info_hashes)) {
    LOG_WARN(QStringLiteral("SessionWorker addMagnet skipped: duplicate info-hash"));
    emitDuplicateRejected(ctx, atp.info_hashes);
    return true;
  }
  atp.save_path = c.savePath.toStdString();
  const QStringList cleanTrackers = pfd::base::sanitizeTrackers(c.trackers);
  for (const auto& tracker : cleanTrackers) {
    atp.trackers.push_back(tracker.toStdString());
  }
  atp.flags |= libtorrent::torrent_flags::auto_managed;
  ses.async_add_torrent(std::move(atp));
  return true;
}

bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::PrepareMagnetMetadataCmd& c) {
  const QString trimmed = c.uri.trimmed();
  if (c.done == nullptr) {
    return true;
  }
  const auto valErr = pfd::base::validateMagnetUri(trimmed);
  if (valErr.hasError()) {
    LOG_ERROR(QStringLiteral("PrepareMagnetMetadata rejected: %1").arg(valErr.message()));
    c.done->set_value(std::nullopt);
    return true;
  }
  libtorrent::error_code ec;
  libtorrent::add_torrent_params atp = libtorrent::parse_magnet_uri(trimmed.toStdString(), ec);
  if (ec) {
    c.done->set_value(std::nullopt);
    return true;
  }
  if (hasActiveTaskForInfoHashes(ctx, atp.info_hashes)) {
    LOG_WARN(QStringLiteral("PrepareMagnetMetadata skipped: duplicate info-hash"));
    emitDuplicateRejected(ctx, atp.info_hashes);
    c.done->set_value(std::nullopt);
    return true;
  }
  const auto id = session_ids::taskIdFromInfoHashes(atp.info_hashes);
  const QString key = session_ids::taskIdKey(id);
  ctx.pendingMagnetMeta[key] = c.done;

  // 临时加入 magnet：让它连接以获取 metadata，但不请求数据块
  atp.save_path = c.tempSavePath.toStdString();
  atp.flags |= libtorrent::torrent_flags::auto_managed;
  atp.flags |= libtorrent::torrent_flags::upload_mode;
  ses.async_add_torrent(std::move(atp));
  return true;
}

bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::CancelPreparedMagnetCmd& c) {
  const QString key = session_ids::taskIdKey(c.taskId);
  const auto it = ctx.handlesByTaskId.find(key);
  if (it != ctx.handlesByTaskId.end()) {
    ses.remove_torrent(it->second);
  }
  ctx.handlesByTaskId.erase(key);
  ctx.preparedTorrentInfo.erase(key);
  ctx.pendingMagnetMeta.erase(key);
  return true;
}

bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::FinalizePreparedMagnetCmd& c) {
  const QString key = session_ids::taskIdKey(c.taskId);
  auto itTi = ctx.preparedTorrentInfo.find(key);
  if (itTi == ctx.preparedTorrentInfo.end() || !itTi->second) {
    return true;
  }

  const auto it = ctx.handlesByTaskId.find(key);
  if (it != ctx.handlesByTaskId.end()) {
    ses.remove_torrent(it->second);
  }
  ctx.handlesByTaskId.erase(key);

  libtorrent::add_torrent_params atp;
  atp.ti = pfd::lt::make_add_torrent_torrent_info(*itTi->second);
  atp.save_path = c.savePath.toStdString();
  const QStringList cleanTrackers = pfd::base::sanitizeTrackers(c.trackers);
  for (const auto& tracker : cleanTrackers) {
    atp.trackers.push_back(tracker.toStdString());
  }
  atp.flags |= libtorrent::torrent_flags::auto_managed;
  if (!c.opts.start_torrent) {
    atp.flags |= libtorrent::torrent_flags::paused;
  }
  if (c.opts.stop_when_ready) {
    atp.flags |= libtorrent::torrent_flags::stop_when_ready;
  }
  if (c.opts.sequential_download) {
    atp.flags |= libtorrent::torrent_flags::sequential_download;
  }
  if (c.opts.skip_hash_check) {
    atp.flags |= libtorrent::torrent_flags::no_verify_files;
  }
  if (!c.opts.file_priorities.empty()) {
    std::vector<libtorrent::download_priority_t> prios;
    prios.reserve(c.opts.file_priorities.size());
    for (auto p : c.opts.file_priorities) {
      prios.emplace_back(libtorrent::download_priority_t{p});
    }
    atp.file_priorities = std::move(prios);
  }
  ctx.pendingAddOpts[key] = c.opts;
  ctx.preparedTorrentInfo.erase(itTi);
  ses.async_add_torrent(std::move(atp));
  return true;
}

bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::AddTorrentFileCmd& c) {
  const auto fileErr = pfd::base::validateTorrentFilePath(c.filePath);
  if (fileErr.hasError()) {
    LOG_ERROR(QStringLiteral("SessionWorker torrent file rejected: %1").arg(fileErr.message()));
    return true;
  }
  libtorrent::error_code ec;
  libtorrent::add_torrent_params atp;
  atp.ti = pfd::lt::make_add_torrent_torrent_info(c.filePath.toStdString(), ec);
  if (ec || !atp.ti) {
    LOG_ERROR(QString("SessionWorker load torrent file failed: %1")
                  .arg(QString::fromStdString(ec.message())));
    return true;
  }
  if (hasActiveTaskForInfoHashes(ctx, atp.ti->info_hashes())) {
    LOG_WARN(QStringLiteral("SessionWorker addTorrentFile skipped: duplicate info-hash"));
    emitDuplicateRejected(ctx, atp.ti->info_hashes());
    return true;
  }
  atp.save_path = c.savePath.toStdString();
  const QStringList cleanTrackers = pfd::base::sanitizeTrackers(c.trackers);
  for (const auto& tracker : cleanTrackers) {
    atp.trackers.push_back(tracker.toStdString());
  }
  atp.flags |= libtorrent::torrent_flags::auto_managed;
  if (c.has_opts) {
    if (!c.opts.start_torrent) {
      atp.flags |= libtorrent::torrent_flags::paused;
    }
    if (c.opts.stop_when_ready) {
      atp.flags |= libtorrent::torrent_flags::stop_when_ready;
    }
    if (c.opts.sequential_download) {
      atp.flags |= libtorrent::torrent_flags::sequential_download;
    }
    if (c.opts.skip_hash_check) {
      atp.flags |= libtorrent::torrent_flags::no_verify_files;
    }
    if (!c.opts.file_priorities.empty()) {
      std::vector<libtorrent::download_priority_t> prios;
      prios.reserve(c.opts.file_priorities.size());
      for (auto p : c.opts.file_priorities) {
        prios.emplace_back(libtorrent::download_priority_t{p});
      }
      atp.file_priorities = std::move(prios);
    }
    const auto id = session_ids::taskIdFromInfoHashes(atp.ti->info_hashes());
    ctx.pendingAddOpts[session_ids::taskIdKey(id)] = c.opts;
  }
  ses.async_add_torrent(std::move(atp));
  return true;
}

bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::AddFromResumeDataCmd& c) {
  if (c.resumeData.isEmpty()) {
    LOG_ERROR(QStringLiteral("[lt.worker] AddFromResumeData ignored: empty data."));
    return true;
  }
  libtorrent::error_code ec;
  libtorrent::add_torrent_params atp = libtorrent::read_resume_data(
      libtorrent::span<const char>(c.resumeData.constData(), c.resumeData.size()), ec);
  if (ec) {
    LOG_ERROR(QStringLiteral("[lt.worker] read_resume_data failed: %1")
                  .arg(QString::fromStdString(ec.message())));
    return true;
  }
  if (hasActiveTaskForInfoHashes(ctx, atp.info_hashes)) {
    LOG_WARN(QStringLiteral("[lt.worker] AddFromResumeData skipped: duplicate info-hash"));
    emitDuplicateRejected(ctx, atp.info_hashes);
    return true;
  }
  if (c.startPaused) {
    atp.flags |= libtorrent::torrent_flags::paused;
    atp.flags &= ~libtorrent::torrent_flags::auto_managed;
  } else {
    atp.flags &= ~libtorrent::torrent_flags::paused;
    atp.flags |= libtorrent::torrent_flags::auto_managed;
  }
  ses.async_add_torrent(std::move(atp));
  return true;
}

}  // namespace pfd::lt::session_ops
