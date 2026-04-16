#ifndef PFD_BASE_THREADING_H
#define PFD_BASE_THREADING_H

#include <QtCore/QMetaObject>
#include <QtCore/QObject>

#include <functional>

namespace pfd::base {

/// 将 fn 投递到 context 所在线程的事件循环执行（QueuedConnection）
/// 调用方需确保 context 存活至 fn 执行完毕
[[nodiscard]] bool runOn(QObject* context, std::function<void()> fn);

}  // namespace pfd::base

#endif  // PFD_BASE_THREADING_H
