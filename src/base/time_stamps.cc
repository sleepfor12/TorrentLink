#include "base/time_stamps.h"

#include <QtCore/QDateTime>

namespace pfd::base {

qint64 currentMs() {
  return QDateTime::currentMSecsSinceEpoch();
}

bool isValidMs(qint64 ms) {
  return ms != invalidMs() && ms >= 0;
}

qint64 fromDateTime(const QDateTime& dt) {
  if (!dt.isValid()) {
    return invalidMs();
  }
  return dt.toMSecsSinceEpoch();
}

QDateTime toDateTime(qint64 ms) {
  if (!isValidMs(ms)) {
    return QDateTime();
  }
  return QDateTime::fromMSecsSinceEpoch(ms, Qt::UTC);
}

}  // namespace pfd::base
