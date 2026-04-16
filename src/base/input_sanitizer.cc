#include "base/input_sanitizer.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtCore/QUrl>

namespace pfd::base {

namespace {

bool containsNullByte(const QString& s) {
  return s.contains(QChar(0));
}

bool hasPathTraversal(const QString& path) {
  auto containsTraversalSegment = [](const QString& p) {
    const auto parts = p.split(QRegularExpression(QStringLiteral(R"([\\/]+)")), Qt::SkipEmptyParts);
    for (const auto& segment : parts) {
      if (segment == QStringLiteral("..")) {
        return true;
      }
    }
    return false;
  };

  if (containsTraversalSegment(path)) {
    return true;
  }
  const QString cleaned = QDir::cleanPath(path);
  return containsTraversalSegment(cleaned);
}

const QStringList& allowedTrackerSchemes() {
  static const QStringList schemes = {
      QStringLiteral("http"),
      QStringLiteral("https"),
      QStringLiteral("udp"),
      QStringLiteral("wss"),
  };
  return schemes;
}

}  // namespace

Error validateMagnetUri(const QString& uri) {
  const QString trimmed = uri.trimmed();
  if (trimmed.isEmpty()) {
    return Error(ErrorCode::kInvalidMagnet, QStringLiteral("Magnet URI is empty"));
  }
  if (trimmed.length() > kMaxMagnetUriLength) {
    return Error(ErrorCode::kInvalidMagnet,
                 QStringLiteral("Magnet URI exceeds maximum length (%1 > %2)")
                     .arg(trimmed.length())
                     .arg(kMaxMagnetUriLength));
  }
  if (containsNullByte(trimmed)) {
    return Error(ErrorCode::kInvalidMagnet, QStringLiteral("Magnet URI contains null bytes"));
  }
  if (!trimmed.startsWith(QStringLiteral("magnet:?"), Qt::CaseInsensitive)) {
    return Error(ErrorCode::kInvalidMagnet,
                 QStringLiteral("Magnet URI must start with \"magnet:?\""));
  }
  return Error();
}

Error validateTrackerUrl(const QString& url) {
  const QString trimmed = url.trimmed();
  if (trimmed.isEmpty()) {
    return Error();
  }
  if (trimmed.length() > kMaxTrackerUrlLength) {
    return Error(ErrorCode::kInvalidArgument,
                 QStringLiteral("Tracker URL exceeds maximum length (%1 > %2)")
                     .arg(trimmed.length())
                     .arg(kMaxTrackerUrlLength));
  }
  if (containsNullByte(trimmed)) {
    return Error(ErrorCode::kInvalidArgument, QStringLiteral("Tracker URL contains null bytes"));
  }
  const QUrl parsed(trimmed, QUrl::StrictMode);
  if (!parsed.isValid()) {
    return Error(ErrorCode::kInvalidArgument,
                 QStringLiteral("Tracker URL is malformed: %1").arg(trimmed));
  }
  const QString scheme = parsed.scheme().toLower();
  if (!allowedTrackerSchemes().contains(scheme)) {
    return Error(ErrorCode::kInvalidArgument,
                 QStringLiteral("Tracker URL scheme \"%1\" not allowed "
                                "(permitted: http, https, udp, wss)")
                     .arg(scheme));
  }
  return Error();
}

Error validateTrackerList(const QStringList& trackers) {
  if (trackers.size() > kMaxTrackersPerTask) {
    return Error(ErrorCode::kInvalidArgument, QStringLiteral("Too many trackers (%1 > %2)")
                                                  .arg(trackers.size())
                                                  .arg(kMaxTrackersPerTask));
  }
  for (const auto& t : trackers) {
    const auto err = validateTrackerUrl(t);
    if (err.hasError()) {
      return err;
    }
  }
  return Error();
}

Error validatePath(const QString& path) {
  const QString trimmed = path.trimmed();
  if (trimmed.isEmpty()) {
    return Error(ErrorCode::kInvalidSavePath, QStringLiteral("Path is empty"));
  }
  if (trimmed.length() > kMaxPathLength) {
    return Error(ErrorCode::kInvalidSavePath,
                 QStringLiteral("Path exceeds maximum length (%1 > %2)")
                     .arg(trimmed.length())
                     .arg(kMaxPathLength));
  }
  if (containsNullByte(trimmed)) {
    return Error(ErrorCode::kInvalidSavePath, QStringLiteral("Path contains null bytes"));
  }
  if (hasPathTraversal(trimmed)) {
    return Error(ErrorCode::kInvalidSavePath,
                 QStringLiteral("Path contains \"..\" traversal component"));
  }
  return Error();
}

Error validateTorrentFilePath(const QString& path) {
  auto err = validatePath(path);
  if (err.hasError()) {
    return Error(ErrorCode::kInvalidTorrentPath, err.message());
  }
  const QString lower = path.trimmed().toLower();
  if (!lower.endsWith(QStringLiteral(".torrent"))) {
    return Error(ErrorCode::kInvalidTorrentPath,
                 QStringLiteral("File does not have .torrent extension"));
  }
  return Error();
}

Error validateCategory(const QString& category) {
  const QString trimmed = category.trimmed();
  if (trimmed.isEmpty()) {
    return Error();
  }
  if (trimmed.length() > kMaxCategoryLength) {
    return Error(ErrorCode::kInvalidArgument, QStringLiteral("Category name too long (%1 > %2)")
                                                  .arg(trimmed.length())
                                                  .arg(kMaxCategoryLength));
  }
  if (containsNullByte(trimmed)) {
    return Error(ErrorCode::kInvalidArgument, QStringLiteral("Category contains null bytes"));
  }
  return Error();
}

Error validateTagsCsv(const QString& tagsCsv) {
  if (tagsCsv.trimmed().isEmpty()) {
    return Error();
  }
  const auto tags = tagsCsv.split(QLatin1Char(','), Qt::SkipEmptyParts);
  if (tags.size() > kMaxTagsCount) {
    return Error(ErrorCode::kInvalidArgument,
                 QStringLiteral("Too many tags (%1 > %2)").arg(tags.size()).arg(kMaxTagsCount));
  }
  for (const auto& t : tags) {
    if (t.trimmed().length() > kMaxTagLength) {
      return Error(ErrorCode::kInvalidArgument, QStringLiteral("Tag \"%1\" exceeds max length (%2)")
                                                    .arg(t.trimmed().left(32))
                                                    .arg(kMaxTagLength));
    }
  }
  return Error();
}

QStringList sanitizeTrackers(const QStringList& trackers) {
  QStringList out;
  QSet<QString> seen;
  for (const auto& t : trackers) {
    const QString trimmed = t.trimmed();
    if (trimmed.isEmpty() || seen.contains(trimmed)) {
      continue;
    }
    if (validateTrackerUrl(trimmed).isOk()) {
      seen.insert(trimmed);
      out.push_back(trimmed);
    }
  }
  return out;
}

}  // namespace pfd::base
