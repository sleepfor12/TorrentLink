#ifndef PFD_BASE_DEBOUNCE_H
#define PFD_BASE_DEBOUNCE_H

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QtGlobal>

#include <functional>

namespace pfd::base {

/// 去抖器：输入停止 delayMs 后再触发回调，用于搜索框等场景。
class Debouncer : public QObject {
  Q_OBJECT
public:
  explicit Debouncer(QObject* parent, qint64 delayMs);

  /// 取消待执行的，重新计时；delayMs 后执行 fn
  void schedule(std::function<void()> fn);

  /// 取消待执行的触发
  void cancel();

  void setDelay(qint64 delayMs);

private:
  void onTimeout();

  qint64 delay_ms_;
  std::function<void()> pending_fn_;
  QTimer timer_;
};

}  // namespace pfd::base

#endif  // PFD_BASE_DEBOUNCE_H
