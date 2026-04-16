#include "base/format.h"

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <cmath>
#include <limits>

namespace pfd::base {

namespace {

struct Unit {
  qint64 factor;
  const char* label;
};

constexpr Unit kSizeUnits[] = {
    {1LL, " B"},
    {1024LL, " KB"},
    {1024LL * 1024, " MB"},
    {1024LL * 1024 * 1024, " GB"},
    {1024LL * 1024 * 1024 * 1024, " TB"},
};

constexpr Unit kSizeUnitsIec[] = {
    {1LL, " B"},
    {1024LL, " KiB"},
    {1024LL * 1024, " MiB"},
    {1024LL * 1024 * 1024, " GiB"},
    {1024LL * 1024 * 1024 * 1024, " TiB"},
};

constexpr Unit kBitsUnits[] = {
    {1LL, " bps"},
    {1000LL, " Kbps"},
    {1000LL * 1000, " Mbps"},
    {1000LL * 1000 * 1000, " Gbps"},
};

constexpr int kSizeUnitCount = sizeof(kSizeUnits) / sizeof(kSizeUnits[0]);
constexpr int kSizeUnitIecCount = sizeof(kSizeUnitsIec) / sizeof(kSizeUnitsIec[0]);
constexpr int kBitsUnitCount = sizeof(kBitsUnits) / sizeof(kBitsUnits[0]);

QString formatQuantity(qint64 value, const Unit* units, int unitCount) {
  if (value < 0) {
    value = 0;
  }
  int i = unitCount - 1;
  while (i > 0 && value < units[i].factor) {
    --i;
  }
  const qint64 factor = units[i].factor;
  const qint64 whole = value / factor;
  const qint64 frac = (value % factor) * 10 / factor;
  if (factor >= 1024 && frac > 0) {
    return QStringLiteral("%1.%2%3").arg(whole).arg(frac).arg(QString::fromUtf8(units[i].label));
  }
  return QStringLiteral("%1%2").arg(whole).arg(QString::fromUtf8(units[i].label));
}

}  // namespace

QString formatBytes(qint64 bytes) {
  return formatQuantity(bytes, kSizeUnits, kSizeUnitCount);
}

QString formatBytesIec(qint64 bytes) {
  return formatQuantity(bytes, kSizeUnitsIec, kSizeUnitIecCount);
}

QString formatRate(qint64 bytesPerSecond) {
  return formatBytes(bytesPerSecond) + QStringLiteral("/s");
}

QString formatRateBits(qint64 bitsPerSecond) {
  return formatQuantity(bitsPerSecond, kBitsUnits, kBitsUnitCount);
}

QString formatDuration(qint64 seconds) {
  if (seconds < 0) {
    seconds = 0;
  }
  const qint64 d = seconds / 86400;
  const qint64 h = (seconds % 86400) / 3600;
  const qint64 m = (seconds % 3600) / 60;
  const qint64 s = seconds % 60;

  QStringList parts;
  if (d > 0) {
    parts << QStringLiteral("%1d").arg(d);
  }
  if (h > 0) {
    parts << QStringLiteral("%1h").arg(h);
  }
  if (m > 0) {
    parts << QStringLiteral("%1m").arg(m);
  }
  if (s > 0 || parts.isEmpty()) {
    parts << QStringLiteral("%1s").arg(s);
  }
  return parts.join(QStringLiteral(" "));
}

QString formatPercent(double value) {
  if (value <= 0.0) {
    return QStringLiteral("0.0%");
  }
  if (value >= 1.0) {
    return QStringLiteral("100.0%");
  }
  return QStringLiteral("%1%").arg(value * 100.0, 0, 'f', 1);
}

QString formatRatio(double ratio) {
  if (std::isnan(ratio) || std::isinf(ratio) || ratio < 0.0 ||
      ratio > static_cast<double>(std::numeric_limits<qint64>::max())) {
    return QStringLiteral("∞");
  }
  if (ratio >= 1000.0) {
    return QStringLiteral("∞");  // 超大比值视为无穷
  }
  return QStringLiteral("%1").arg(ratio, 0, 'f', 1);
}

}  // namespace pfd::base
