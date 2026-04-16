#ifndef PFD_BASE_FS_UTILS_H
#define PFD_BASE_FS_UTILS_H

#include <QtCore/QString>
#include <QtCore/QtGlobal>

namespace pfd::base {

[[nodiscard]] bool fileExists(const QString& path);
[[nodiscard]] bool dirExists(const QString& path);
[[nodiscard]] qint64 fileSize(const QString& path);        // not found => -1
[[nodiscard]] qint64 availableBytes(const QString& path);  // failed => -1

}  // namespace pfd::base

#endif  // PFD_BASE_FS_UTILS_H
