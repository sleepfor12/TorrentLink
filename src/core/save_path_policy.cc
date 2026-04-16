#include "core/save_path_policy.h"

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

namespace pfd::core {

void SavePathPolicy::setDefaultDownloadDir(const QString& dir) {
  default_dir_ = dir.trimmed();
}

QString SavePathPolicy::resolve(const QString& input) const {
  QString p = input.trimmed();
  if (p.isEmpty()) p = default_dir_;
  if (p.isEmpty()) {
    p = QDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))
            .filePath(QStringLiteral("p2p"));
  }
  p = QDir::cleanPath(p);
  ensureExists(p);
  return p;
}

QString SavePathPolicy::applyLayout(const QString& basePath,
                                    const QString& torrentName,
                                    ContentLayout layout) const {
  switch (layout) {
    case ContentLayout::kCreateSubfolder:
      return QDir(basePath).filePath(torrentName);
    case ContentLayout::kNoSubfolder:
    case ContentLayout::kOriginal:
    default:
      return basePath;
  }
}

void SavePathPolicy::ensureExists(const QString& path) {
  QDir dir;
  if (!dir.exists(path)) dir.mkpath(path);
}

}  // namespace pfd::core
