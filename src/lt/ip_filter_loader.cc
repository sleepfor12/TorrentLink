#include "lt/ip_filter_loader.h"

#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtEndian>
#include <QtNetwork/QHostAddress>

#include <libtorrent/address.hpp>

#include <array>
#include <cstring>
#include <utility>

namespace pfd::lt {
namespace {

libtorrent::address addressFromRaw6(const std::array<unsigned char, 16>& raw) {
  return libtorrent::address(libtorrent::address_v6(raw));
}

void ipv6ApplyPrefixRange(const std::array<unsigned char, 16>& ip, int prefix,
                          std::array<unsigned char, 16>* first, std::array<unsigned char, 16>* last) {
  *first = ip;
  *last = ip;
  if (prefix <= 0) {
    first->fill(0);
    last->fill(0xFF);
    return;
  }
  if (prefix >= 128) {
    return;
  }
  for (int k = prefix; k < 128; ++k) {
    const int bi = k / 8;
    const int bo = k % 8;
    const unsigned char mask = static_cast<unsigned char>(1u << (7 - bo));
    (*first)[bi] = static_cast<unsigned char>((*first)[bi] & static_cast<unsigned char>(~mask));
    (*last)[bi] = static_cast<unsigned char>((*last)[bi] | mask);
  }
}

bool ipv4ToLtAddress(quint32 hostOrder, libtorrent::address* out) {
  const quint32 be = qToBigEndian(hostOrder);
  libtorrent::address_v4 a4;
  std::memcpy(&a4, &be, sizeof(be));
  *out = libtorrent::address(a4);
  return true;
}

bool parseDottedIPv4(QStringView s, libtorrent::address* first, libtorrent::address* last) {
  QHostAddress a;
  if (!a.setAddress(s.trimmed().toString()) || a.protocol() != QAbstractSocket::IPv4Protocol) {
    return false;
  }
  const quint32 h = a.toIPv4Address();
  ipv4ToLtAddress(h, first);
  *last = *first;
  return true;
}

QString stripOptionalBrackets(QStringView s) {
  QString t = s.trimmed().toString();
  if (t.size() >= 2 && t.front() == QLatin1Char('[') && t.back() == QLatin1Char(']')) {
    return t.mid(1, t.size() - 2);
  }
  return t;
}

bool parseAnySingleIp(QStringView s, libtorrent::address* out) {
  const QString token = stripOptionalBrackets(s);
  QHostAddress a;
  if (!a.setAddress(token)) {
    return false;
  }
  if (a.protocol() == QAbstractSocket::IPv4Protocol) {
    const quint32 h = a.toIPv4Address();
    return ipv4ToLtAddress(h, out);
  }
  if (a.protocol() == QAbstractSocket::IPv6Protocol) {
    boost::system::error_code ec;
    const auto lt = libtorrent::make_address(token.toStdString(), ec);
    if (ec || !lt.is_v6()) {
      return false;
    }
    *out = lt;
    return true;
  }
  return false;
}

bool appendCidrIpv4(QStringView line, libtorrent::ip_filter* out) {
  const int slash = line.indexOf(QLatin1Char('/'));
  if (slash <= 0) {
    return false;
  }
  const QStringView ipPart = line.left(slash).trimmed();
  bool ok = false;
  const int prefix = line.mid(slash + 1).trimmed().toString().toInt(&ok);
  if (!ok || prefix < 0 || prefix > 32) {
    return false;
  }
  QHostAddress base;
  if (!base.setAddress(ipPart.toString()) || base.protocol() != QAbstractSocket::IPv4Protocol) {
    return false;
  }
  const quint32 ip = base.toIPv4Address();
  if (prefix == 0) {
    libtorrent::address f;
    libtorrent::address l;
    ipv4ToLtAddress(0u, &f);
    ipv4ToLtAddress(0xFFFFFFFFu, &l);
    out->add_rule(f, l, libtorrent::ip_filter::blocked);
    return true;
  }
  const quint32 mask = static_cast<quint32>(0xFFFFFFFFu << (32 - prefix));
  const quint32 firstHost = ip & mask;
  const quint32 lastHost = firstHost | ~mask;
  libtorrent::address fa;
  libtorrent::address la;
  ipv4ToLtAddress(firstHost, &fa);
  ipv4ToLtAddress(lastHost, &la);
  out->add_rule(fa, la, libtorrent::ip_filter::blocked);
  return true;
}

bool appendCidrIpv6(QStringView line, libtorrent::ip_filter* out) {
  const int slash = line.indexOf(QLatin1Char('/'));
  if (slash <= 0) {
    return false;
  }
  const QString ipStr = stripOptionalBrackets(line.left(slash).trimmed());
  bool ok = false;
  const int prefix = line.mid(slash + 1).trimmed().toString().toInt(&ok);
  if (!ok || prefix < 0 || prefix > 128) {
    return false;
  }
  boost::system::error_code ec;
  const libtorrent::address base = libtorrent::make_address(ipStr.toStdString(), ec);
  if (ec || !base.is_v6()) {
    return false;
  }
  std::array<unsigned char, 16> raw = base.to_v6().to_bytes();
  std::array<unsigned char, 16> first{};
  std::array<unsigned char, 16> last{};
  ipv6ApplyPrefixRange(raw, prefix, &first, &last);
  out->add_rule(addressFromRaw6(first), addressFromRaw6(last), libtorrent::ip_filter::blocked);
  return true;
}

bool appendRangeIpv4(QStringView line, libtorrent::ip_filter* out) {
  static const QString sep = QStringLiteral(" - ");
  const QString s = line.trimmed().toString();
  const int pos = s.indexOf(sep);
  if (pos < 0) {
    return false;
  }
  libtorrent::address a;
  libtorrent::address b;
  if (!parseDottedIPv4(QStringView{s}.left(pos), &a, &a)) {
    return false;
  }
  if (!parseDottedIPv4(QStringView{s}.mid(pos + sep.size()), &b, &b)) {
    return false;
  }
  const auto ba = a.to_v4().to_bytes();
  const auto bb = b.to_v4().to_bytes();
  if (std::memcmp(ba.data(), bb.data(), 4) > 0) {
    std::swap(a, b);
  }
  out->add_rule(a, b, libtorrent::ip_filter::blocked);
  return true;
}

bool appendRangeIpv6(QStringView line, libtorrent::ip_filter* out) {
  static const QString sep = QStringLiteral(" - ");
  const QString s = line.trimmed().toString();
  const int pos = s.indexOf(sep);
  if (pos < 0) {
    return false;
  }
  libtorrent::address a;
  libtorrent::address b;
  if (!parseAnySingleIp(QStringView{s}.left(pos), &a) || !a.is_v6()) {
    return false;
  }
  if (!parseAnySingleIp(QStringView{s}.mid(pos + sep.size()), &b) || !b.is_v6()) {
    return false;
  }
  const auto ba = a.to_v6().to_bytes();
  const auto bb = b.to_v6().to_bytes();
  if (std::memcmp(ba.data(), bb.data(), 16) > 0) {
    std::swap(a, b);
  }
  out->add_rule(a, b, libtorrent::ip_filter::blocked);
  return true;
}

bool lineLooksLikeIpv6(QStringView t) {
  if (t.contains(QLatin1Char(':'))) {
    return true;
  }
  if (t.startsWith(QLatin1Char('['))) {
    return true;
  }
  return false;
}

}  // namespace

bool appendIpFilterLine(QStringView line, libtorrent::ip_filter* out) {
  QStringView t = line.trimmed();
  if (t.isEmpty() || t.startsWith(QLatin1Char('#'))) {
    return true;
  }
  if (t.contains(QLatin1Char('/'))) {
    if (lineLooksLikeIpv6(t.left(t.indexOf(QLatin1Char('/'))))) {
      return appendCidrIpv6(t, out);
    }
    if (appendCidrIpv4(t, out)) {
      return true;
    }
    return appendCidrIpv6(t, out);
  }
  if (t.contains(QStringLiteral(" - "))) {
    const QString s = t.toString();
    const int pos = s.indexOf(QStringLiteral(" - "));
    const QStringView left = QStringView{s}.left(pos).trimmed();
    const QStringView right = QStringView{s}.mid(pos + 3).trimmed();
    const bool left6 = lineLooksLikeIpv6(left);
    const bool right6 = lineLooksLikeIpv6(right);
    if (left6 || right6) {
      if (!left6 || !right6) {
        return false;
      }
      return appendRangeIpv6(t, out);
    }
    return appendRangeIpv4(t, out);
  }
  libtorrent::address a;
  if (!parseAnySingleIp(t, &a)) {
    return false;
  }
  out->add_rule(a, a, libtorrent::ip_filter::blocked);
  return true;
}

bool loadIpFilterFile(const QString& path, libtorrent::ip_filter* out, int* rules_applied,
                      QString* error) {
  if (out == nullptr) {
    if (error != nullptr) {
      *error = QStringLiteral("out==null");
    }
    return false;
  }
  *out = libtorrent::ip_filter{};
  QFile f(path);
  if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    if (error != nullptr) {
      *error = QStringLiteral("cannot open file");
    }
    return false;
  }
  int applied = 0;
  QTextStream ts(&f);
  while (!ts.atEnd()) {
    const QString raw = ts.readLine();
    QStringView v(raw);
    const auto trimmed = v.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#'))) {
      continue;
    }
    if (appendIpFilterLine(trimmed, out)) {
      ++applied;
    }
  }
  if (rules_applied != nullptr) {
    *rules_applied = applied;
  }
  if (error != nullptr) {
    error->clear();
  }
  return true;
}

}  // namespace pfd::lt
