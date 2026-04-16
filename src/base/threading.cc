#include "base/threading.h"

#include <QtCore/QMetaObject>
#include <QtCore/QObject>

namespace pfd::base {

bool runOn(QObject* context, std::function<void()> fn) {
  if (!context || !fn) {
    return false;
  }
  return QMetaObject::invokeMethod(context, [fn]() { fn(); }, Qt::QueuedConnection);
}

}  // namespace pfd::base
