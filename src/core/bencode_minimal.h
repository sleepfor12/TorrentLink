#ifndef PFD_CORE_BENCODE_MINIMAL_H
#define PFD_CORE_BENCODE_MINIMAL_H

#include <QtCore/QByteArray>
#include <QtCore/QString>

namespace pfd::core::bencode {

[[nodiscard]] QByteArray encodeInt(qint64 v);
[[nodiscard]] QByteArray encodeString(const QByteArray& s);
[[nodiscard]] QByteArray encodeAnnounceFailure(const QString& reason);
[[nodiscard]] QByteArray encodeAnnounceOk(int interval_sec, int min_interval_sec, int complete,
                                          int incomplete, const QByteArray& peers_compact);

}  // namespace pfd::core::bencode

#endif  // PFD_CORE_BENCODE_MINIMAL_H
