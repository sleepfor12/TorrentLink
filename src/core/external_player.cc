#include "core/external_player.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtGui/QDesktopServices>
#include <QtCore/QUrl>

#include "base/process_utils.h"
#include "core/logger.h"

namespace pfd::core {

ExternalPlayer::Result ExternalPlayer::openWithDefault(const QString& file_path) {
  if (!QFileInfo::exists(file_path)) {
    LOG_WARN(QStringLiteral("[player] File not found: %1").arg(file_path));
    return Result::FileNotFound;
  }
  const bool ok = QDesktopServices::openUrl(QUrl::fromLocalFile(file_path));
  if (!ok) {
    LOG_WARN(QStringLiteral("[player] Failed to open with default app: %1").arg(file_path));
    return Result::LaunchFailed;
  }
  LOG_INFO(QStringLiteral("[player] Opened with default: %1").arg(file_path));
  return Result::Ok;
}

ExternalPlayer::Result ExternalPlayer::openWithCommand(const QString& command,
                                                       const QString& file_path) {
  if (!QFileInfo::exists(file_path)) {
    LOG_WARN(QStringLiteral("[player] File not found: %1").arg(file_path));
    return Result::FileNotFound;
  }
  const auto detached = pfd::base::buildDetachedCommand(command, {file_path});
  if (!detached.isValid()) {
    LOG_WARN(QStringLiteral("[player] Empty player command"));
    return Result::LaunchFailed;
  }
  const bool ok = QProcess::startDetached(detached.program, detached.arguments);
  if (!ok) {
    LOG_WARN(QStringLiteral("[player] Failed to launch \"%1\" with \"%2\"")
                 .arg(command, file_path));
    return Result::LaunchFailed;
  }
  LOG_INFO(QStringLiteral("[player] Launched \"%1\" \"%2\"").arg(command, file_path));
  return Result::Ok;
}

ExternalPlayer::Result ExternalPlayer::openFolder(const QString& path) {
  QString dirPath = path;
  QFileInfo fi(path);
  if (fi.isFile()) dirPath = fi.absolutePath();

  if (!QDir(dirPath).exists()) {
    LOG_WARN(QStringLiteral("[player] Directory not found: %1").arg(dirPath));
    return Result::FileNotFound;
  }
  const bool ok = QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
  if (!ok) {
    LOG_WARN(QStringLiteral("[player] Failed to open folder: %1").arg(dirPath));
    return Result::LaunchFailed;
  }
  LOG_INFO(QStringLiteral("[player] Opened folder: %1").arg(dirPath));
  return Result::Ok;
}

}  // namespace pfd::core
