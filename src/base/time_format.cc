#include "base/time_format.h"

#include <QtCore/QDateTime>

#include "base/time_stamps.h"

namespace pfd::base {

QString formatIso8601(qint64 ms) {
  if (!isValidMs(ms)) {
    return QString();
  }
  return toDateTime(ms).toString(Qt::ISODateWithMs);
}

QString formatIso8601(const QDateTime& dt) {
  if (!dt.isValid()) {
    return QString();
  }
  return dt.toUTC().toString(Qt::ISODateWithMs);
}

QString formatDateTime(qint64 ms) {
  if (!isValidMs(ms)) {
    return QString();
  }
  const QDateTime utc = toDateTime(ms);
  const QDateTime local = utc.toLocalTime();
  return local.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

QString formatDateTime(const QDateTime& dt) {
  if (!dt.isValid()) {
    return QString();
  }
  const QDateTime local = dt.toLocalTime();
  return local.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

}  // namespace pfd::base
