#ifndef PFD_BASE_TIME_STAMPS_H
#define PFD_BASE_TIME_STAMPS_H

#include <QtCore/QDateTime>
#include <QtCore/QtGlobal>

namespace pfd::base {

/// 当前 UTC 毫秒时间戳（自 Unix 纪元）
[[nodiscard]] qint64 currentMs();

/// 无效/未设置时间的哨兵值，约定为 -1
[[nodiscard]] constexpr qint64 invalidMs() {
  return -1;
}

/// 是否为有效时间戳（非哨兵且在合理范围）
[[nodiscard]] bool isValidMs(qint64 ms);

/// QDateTime → 毫秒；无效 QDateTime 返回 invalidMs()
[[nodiscard]] qint64 fromDateTime(const QDateTime& dt);

/// 毫秒 → QDateTime（UTC）；无效 ms 返回 QDateTime()
[[nodiscard]] QDateTime toDateTime(qint64 ms);

}  // namespace pfd::base

#endif  // PFD_BASE_TIME_STAMPS_H
