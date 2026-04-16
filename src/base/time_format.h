#ifndef PFD_BASE_TIME_FORMAT_H
#define PFD_BASE_TIME_FORMAT_H

#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

namespace pfd::base {

/// ms → ISO 8601 格式（yyyy-MM-ddTHH:mm:ss.zzzZ）；无效 ms 返回空字符串
[[nodiscard]] QString formatIso8601(qint64 ms);
[[nodiscard]] QString formatIso8601(const QDateTime& dt);

/// ms → 本地化友好格式（yyyy-MM-dd HH:mm:ss）；无效 ms 返回空字符串
[[nodiscard]] QString formatDateTime(qint64 ms);
[[nodiscard]] QString formatDateTime(const QDateTime& dt);

}  // namespace pfd::base

#endif  // PFD_BASE_TIME_FORMAT_H
