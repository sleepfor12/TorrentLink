#ifndef PFD_BASE_FORMAT_H
#define PFD_BASE_FORMAT_H

#include <QtCore/QString>
#include <QtCore/QtGlobal>

namespace pfd::base {

/// 字节数 → 可读字符串（如 "1.2 GB"、"500 MB"），1024 进制
[[nodiscard]] QString formatBytes(qint64 bytes);

/// 字节数 → IEC 单位（如 "1.2 GiB"、"500 MiB"），1024 进制
[[nodiscard]] QString formatBytesIec(qint64 bytes);

/// B/s → 可读速率字符串（如 "1.2 MB/s"）
[[nodiscard]] QString formatRate(qint64 bytesPerSecond);

/// bits/s → 可读速率字符串（如 "1.2 Mbps"），1000 进制
[[nodiscard]] QString formatRateBits(qint64 bitsPerSecond);

/// 秒数 → 可读时间间隔（如 "2h 30m"、"5m 12s"）；负值视为 0
[[nodiscard]] QString formatDuration(qint64 seconds);

/// 进度百分比，value ∈ [0, 1]，输出如 "75.3%"
[[nodiscard]] QString formatPercent(double value);

/// 分享率，ratio < 0 或无穷大时返回 "∞"
[[nodiscard]] QString formatRatio(double ratio);

}  // namespace pfd::base

#endif  // PFD_BASE_FORMAT_H
