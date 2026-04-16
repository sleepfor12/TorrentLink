#ifndef PFD_BASE_THROTTLE_H
#define PFD_BASE_THROTTLE_H

#include <QtCore/QtGlobal>

namespace pfd::base {

/// 节流器：按固定间隔判断是否该执行，用于快照刷新频率控制。
class Throttler {
public:
  explicit Throttler(qint64 intervalMs);

  /// 距离上次执行是否已超过间隔；首次调用视为“该执行”
  [[nodiscard]] bool shouldFire(qint64 nowMs) const;

  /// 记录执行时间
  void markFired(qint64 nowMs);

  /// 重置，下次 shouldFire 必为 true
  void reset();

private:
  qint64 interval_ms_;
  qint64 last_fired_ms_;
  bool has_fired_;
};

}  // namespace pfd::base

#endif  // PFD_BASE_THROTTLE_H
