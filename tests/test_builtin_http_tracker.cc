#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>

#include <gtest/gtest.h>

#include "core/bencode_minimal.h"
#include "core/builtin_http_tracker.h"

namespace {

/// BEP 3 info_hash / peer_id are raw 20 bytes; use fixed %HH per byte (do not use
/// QUrl::toPercentEncoding for arbitrary octets — Qt may emit %EF%BF%BD for invalid UTF-8 bytes).
[[nodiscard]] QByteArray percentEncodeBytes(const QByteArray& raw) {
  static const char* hexd = "0123456789ABCDEF";
  QByteArray out;
  out.reserve(raw.size() * 3);
  for (unsigned char b : raw) {
    out += '%';
    out += hexd[b >> 4];
    out += hexd[b & 0xf];
  }
  return out;
}

[[nodiscard]] QByteArray percentDecodeQueryValue(QByteArray enc) {
  QByteArray out;
  out.reserve(enc.size());
  for (int i = 0; i < enc.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(enc[i]);
    if (c == '%' && i + 2 < enc.size()) {
      const auto hex = [](unsigned char ch) -> int {
        if (ch >= '0' && ch <= '9') {
          return static_cast<int>(ch - '0');
        }
        if (ch >= 'A' && ch <= 'F') {
          return static_cast<int>(ch - 'A' + 10);
        }
        if (ch >= 'a' && ch <= 'f') {
          return static_cast<int>(ch - 'a' + 10);
        }
        return -1;
      };
      const int hi = hex(static_cast<unsigned char>(enc[i + 1]));
      const int lo = hex(static_cast<unsigned char>(enc[i + 2]));
      if (hi >= 0 && lo >= 0) {
        out.append(static_cast<char>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }
    if (c == '+') {
      out.append(' ');
      continue;
    }
    out.append(static_cast<char>(c));
  }
  return out;
}

[[nodiscard]] QByteArray queryParamRaw(const QByteArray& qs, const QByteArray& key) {
  const QByteArray needle = key + QByteArrayLiteral("=");
  int from = 0;
  while (from < qs.size()) {
    const int next = qs.indexOf('&', from);
    const QByteArray slice = next < 0 ? qs.mid(from) : qs.mid(from, next - from);
    if (slice.startsWith(needle)) {
      return percentDecodeQueryValue(slice.mid(needle.size()));
    }
    if (next < 0) {
      break;
    }
    from = next + 1;
  }
  return {};
}

[[nodiscard]] QByteArray httpBody(const QByteArray& raw) {
  const int sep = raw.indexOf("\r\n\r\n");
  if (sep < 0) {
    return {};
  }
  return raw.mid(sep + 4);
}

[[nodiscard]] QByteArray buildAnnounceGetRequest(const QByteArray& ih, const QByteArray& peer_id,
                                                 int announce_port, int compact, int left) {
  QByteArray path;
  path += "/announce?info_hash=";
  path += percentEncodeBytes(ih);
  path += "&peer_id=";
  path += percentEncodeBytes(peer_id);
  path += "&port=";
  path += QByteArray::number(announce_port);
  path += "&compact=";
  path += QByteArray::number(compact);
  path += "&left=";
  path += QByteArray::number(left);
  QByteArray req;
  req += "GET ";
  req += path;
  req += " HTTP/1.0\r\n\r\n";
  return req;
}

}  // namespace

TEST(BuiltinHttpTrackerBencode, AnnounceOkEmptyPeers) {
  const QByteArray got = pfd::core::bencode::encodeAnnounceOk(1800, 900, 0, 0, {});
  const QByteArray expected = QByteArrayLiteral(
      "d8:intervali1800e12:min intervali900e8:completei0e10:incompletei0e5:peers0:e");
  EXPECT_EQ(got, expected);
}

TEST(BuiltinHttpTrackerBencode, AnnounceFailure) {
  const QByteArray got = pfd::core::bencode::encodeAnnounceFailure(QStringLiteral("bad"));
  const QByteArray expected = QByteArrayLiteral("d14:failure reason3:bade");
  EXPECT_EQ(got, expected);
}

TEST(BuiltinHttpTrackerQuery, QueryParamRawDecodesBinaryInfoHash) {
  const QByteArray ih(20, '\x02');
  const QByteArray pid(20, '\xcc');
  ASSERT_EQ(percentDecodeQueryValue(percentEncodeBytes(ih)), ih);
  ASSERT_EQ(percentDecodeQueryValue(percentEncodeBytes(pid)), pid);
  QByteArray qs;
  qs += "info_hash=";
  qs += percentEncodeBytes(ih);
  qs += "&peer_id=";
  qs += percentEncodeBytes(pid);
  qs += "&port=5000&compact=1&left=0";
  ASSERT_EQ(queryParamRaw(qs, QByteArrayLiteral("info_hash")), ih);
  ASSERT_EQ(queryParamRaw(qs, QByteArrayLiteral("peer_id")), pid);
}

TEST(BuiltinHttpTrackerBencode, AnnounceOkWithOneCompactPeer) {
  // 127.0.0.1:40000 -> 7f 00 00 01 9c 40
  const QByteArray peers = QByteArray::fromHex("7f0000019c40");
  const QByteArray got = pfd::core::bencode::encodeAnnounceOk(1800, 900, 1, 2, peers);
  EXPECT_TRUE(got.contains(QByteArrayLiteral("5:peers")));
  EXPECT_TRUE(got.contains(peers));
}

TEST(BuiltinHttpTracker, SmokeAnnounceLocalhostTwoPeers) {
  pfd::core::BuiltinHttpTracker tr(nullptr);
  tr.apply(true, 0);
  const quint16 port = tr.serverPort();
  ASSERT_NE(port, 0u);

  const QByteArray ih(20, '\x01');
  const QByteArray pid_a(20, '\xaa');
  const QByteArray pid_b(20, '\xbb');

  {
    QTcpSocket s;
    s.connectToHost(QHostAddress::LocalHost, port);
    ASSERT_TRUE(s.waitForConnected(3000));
    s.write(buildAnnounceGetRequest(ih, pid_a, 40000, 1, 0));
    s.flush();
    ASSERT_TRUE(s.waitForReadyRead(5000));
    QByteArray resp = s.readAll();
    while (s.waitForReadyRead(200)) {
      resp += s.readAll();
    }
    const QByteArray body = httpBody(resp);
    ASSERT_FALSE(body.isEmpty());
    EXPECT_TRUE(body.contains("interval"));
  }

  {
    QTcpSocket s;
    s.connectToHost(QHostAddress::LocalHost, port);
    ASSERT_TRUE(s.waitForConnected(3000));
    s.write(buildAnnounceGetRequest(ih, pid_b, 40001, 1, 100));
    s.flush();
    ASSERT_TRUE(s.waitForReadyRead(5000));
    QByteArray resp = s.readAll();
    while (s.waitForReadyRead(200)) {
      resp += s.readAll();
    }
    const QByteArray body = httpBody(resp);
    ASSERT_FALSE(body.isEmpty());
    // First peer (A) at 127.0.0.1:40000 in compact form (B should see A, not self).
    const QByteArray compact_a = QByteArray::fromHex("7f0000019c40");
    EXPECT_TRUE(body.contains(compact_a)) << QString::fromLatin1(body.toHex()).toStdString();
  }

  tr.apply(false, 0);
}

TEST(BuiltinHttpTracker, RejectsCompactZero) {
  pfd::core::BuiltinHttpTracker tr(nullptr);
  tr.apply(true, 0);
  const quint16 port = tr.serverPort();
  ASSERT_NE(port, 0u);

  const QByteArray ih(20, '\x02');
  const QByteArray pid(20, '\xcc');
  QTcpSocket s;
  s.connectToHost(QHostAddress::LocalHost, port);
  ASSERT_TRUE(s.waitForConnected(3000));
  s.write(buildAnnounceGetRequest(ih, pid, 5000, 0, 0));
  ASSERT_TRUE(s.waitForReadyRead(5000));
  QByteArray resp = s.readAll();
  while (s.waitForReadyRead(200)) {
    resp += s.readAll();
  }
  const QByteArray body = httpBody(resp);
  EXPECT_TRUE(body.contains(QByteArrayLiteral("compact=1 required")))
      << QString::fromLatin1(body.toHex()).toStdString();
  tr.apply(false, 0);
}
