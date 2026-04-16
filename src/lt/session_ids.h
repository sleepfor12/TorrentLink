#ifndef PFD_LT_SESSION_IDS_H
#define PFD_LT_SESSION_IDS_H

#include <QCryptographicHash>
#include <QtCore/QUuid>

#include <libtorrent/info_hash.hpp>

#include <algorithm>
#include <sstream>

#include "base/types.h"

namespace pfd::lt::session_ids {

inline QString taskIdKey(const pfd::base::TaskId& taskId) {
  return taskId.toString(QUuid::WithoutBraces);
}

inline pfd::base::TaskId taskIdFromInfoHashes(const libtorrent::info_hash_t& infoHashes) {
  std::ostringstream oss;
  oss << infoHashes;
  const QString key = QStringLiteral("ih|%1").arg(QString::fromStdString(oss.str()));
  const QByteArray h = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1);
  QByteArray uuidBytes(16, '\0');
  const int n = std::min<int>(16, h.size());
  for (int i = 0; i < n; ++i) {
    uuidBytes[i] = h[i];
  }
  uuidBytes[6] = static_cast<char>((uuidBytes[6] & 0x0F) | 0x50);
  uuidBytes[8] = static_cast<char>((uuidBytes[8] & 0x3F) | 0x80);
  return QUuid::fromRfc4122(uuidBytes);
}

}  // namespace pfd::lt::session_ids

#endif  // PFD_LT_SESSION_IDS_H
