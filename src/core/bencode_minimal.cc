#include "core/bencode_minimal.h"

namespace pfd::core::bencode {

QByteArray encodeInt(qint64 v) {
  return QByteArrayLiteral("i") + QByteArray::number(v) + QByteArrayLiteral("e");
}

QByteArray encodeString(const QByteArray& s) {
  return QByteArray::number(static_cast<qint64>(s.size())) + QByteArrayLiteral(":") + s;
}

QByteArray encodeAnnounceFailure(const QString& reason) {
  const QByteArray utf8 = reason.toUtf8();
  QByteArray d;
  d += 'd';
  d += encodeString(QByteArrayLiteral("failure reason"));
  d += encodeString(utf8);
  d += 'e';
  return d;
}

QByteArray encodeAnnounceOk(int interval_sec, int min_interval_sec, int complete, int incomplete,
                            const QByteArray& peers_compact) {
  QByteArray d;
  d += 'd';
  d += encodeString(QByteArrayLiteral("interval"));
  d += encodeInt(interval_sec);
  d += encodeString(QByteArrayLiteral("min interval"));
  d += encodeInt(min_interval_sec);
  d += encodeString(QByteArrayLiteral("complete"));
  d += encodeInt(complete);
  d += encodeString(QByteArrayLiteral("incomplete"));
  d += encodeInt(incomplete);
  d += encodeString(QByteArrayLiteral("peers"));
  d += encodeString(peers_compact);
  d += 'e';
  return d;
}

}  // namespace pfd::core::bencode
