#include "base/paths.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

namespace pfd::base {

namespace {

constexpr char kAppName[] = "p2p_downloader";

QString ensureTrailingSlash(const QString& path) {
  if (path.isEmpty()) {
    return path;
  }
  const QString cleaned = QDir::cleanPath(path);
  const QChar sep = QDir::separator();
  if (cleaned.endsWith(sep) || cleaned.endsWith(QLatin1Char('/')) ||
      cleaned.endsWith(QLatin1Char('\\'))) {
    return cleaned;
  }
  return cleaned + sep;
}

}  // namespace

QString appDataDir() {
  const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  if (base.isEmpty()) {
    return QString();
  }
  return ensureTrailingSlash(QDir(base).filePath(QLatin1String(kAppName)));
}

QString configDir() {
  const QString base = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
  if (base.isEmpty()) {
    return QString();
  }
  return ensureTrailingSlash(QDir(base).filePath(QLatin1String(kAppName)));
}

QString resumeDir() {
  const QString base = appDataDir();
  if (base.isEmpty()) {
    return QString();
  }
  return ensureTrailingSlash(QDir(base).filePath(QStringLiteral("resume")));
}

bool ensureExists(const QString& path) {
  if (path.isEmpty()) {
    return false;
  }
  QDir dir;
  return dir.mkpath(path);
}

}  // namespace pfd::base
