#include "base/fs_utils.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStorageInfo>

namespace pfd::base {

bool fileExists(const QString& path) {
  const QFileInfo fi(path);
  return fi.exists() && fi.isFile();
}

bool dirExists(const QString& path) {
  const QFileInfo fi(path);
  return fi.exists() && fi.isDir();
}

qint64 fileSize(const QString& path) {
  const QFileInfo fi(path);
  if (!fi.exists() || !fi.isFile()) {
    return -1;
  }
  return fi.size();
}

qint64 availableBytes(const QString& path) {
  QString target = path;
  if (target.isEmpty()) {
    target = QDir::homePath();
  }
  const QStorageInfo storage(target);
  if (!storage.isValid() || !storage.isReady()) {
    return -1;
  }
  return static_cast<qint64>(storage.bytesAvailable());
}

}  // namespace pfd::base
