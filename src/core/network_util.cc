#include "core/network_util.h"

#include <QtNetwork/QNetworkInterface>

namespace pfd::core {
namespace {

[[nodiscard]] bool isPrivateIPv4(const QHostAddress& addr) {
  if (addr.protocol() != QAbstractSocket::IPv4Protocol) {
    return false;
  }
  const quint32 ip = addr.toIPv4Address();
  const quint8 b0 = static_cast<quint8>((ip >> 24) & 0xff);
  const quint8 b1 = static_cast<quint8>((ip >> 16) & 0xff);
  if (b0 == 10) {
    return true;
  }
  if (b0 == 172 && b1 >= 16 && b1 <= 31) {
    return true;
  }
  if (b0 == 192 && b1 == 168) {
    return true;
  }
  return false;
}

[[nodiscard]] bool isUsableLanIPv4(const QHostAddress& addr) {
  if (addr.protocol() != QAbstractSocket::IPv4Protocol) {
    return false;
  }
  if (addr.isLoopback() || addr.isLinkLocal()) {
    return false;
  }
  return true;
}

}  // namespace

QString normalizeBuiltinTrackerBindMode(const QString& raw) {
  const QString v = raw.trimmed().toLower();
  if (v == QStringLiteral("lan")) {
    return QStringLiteral("lan");
  }
  return QStringLiteral("localhost");
}

QHostAddress resolveBuiltinTrackerBindAddress(const QString& bind_mode) {
  if (normalizeBuiltinTrackerBindMode(bind_mode) == QStringLiteral("lan")) {
    return QHostAddress(QHostAddress::AnyIPv4);
  }
  return QHostAddress(QHostAddress::LocalHost);
}

QString primaryPrivateIPv4Address() {
  QString fallback;
  const auto interfaces = QNetworkInterface::allAddresses();
  for (const QHostAddress& addr : interfaces) {
    if (!isUsableLanIPv4(addr)) {
      continue;
    }
    if (isPrivateIPv4(addr)) {
      return addr.toString();
    }
    if (fallback.isEmpty()) {
      fallback = addr.toString();
    }
  }
  return fallback;
}

}  // namespace pfd::core
