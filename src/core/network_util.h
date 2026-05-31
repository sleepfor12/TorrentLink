#ifndef PFD_CORE_NETWORK_UTIL_H
#define PFD_CORE_NETWORK_UTIL_H

#include <QtCore/QString>
#include <QtNetwork/QHostAddress>

namespace pfd::core {

/// 内置 Tracker 绑定范围：`localhost`（仅本机）或 `lan`（所有 IPv4 接口，供局域网访问）。
[[nodiscard]] QString normalizeBuiltinTrackerBindMode(const QString& raw);

[[nodiscard]] QHostAddress resolveBuiltinTrackerBindAddress(const QString& bind_mode);

/// 首选私有 IPv4（10/8、172.16/12、192.168/16），用于日志中的局域网 announce URL。
[[nodiscard]] QString primaryPrivateIPv4Address();

}  // namespace pfd::core

#endif  // PFD_CORE_NETWORK_UTIL_H
