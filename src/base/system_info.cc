#include "base/system_info.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QSysInfo>

namespace pfd::base {

qint64 processId() {
  return static_cast<qint64>(QCoreApplication::applicationPid());
}

QString hostName() {
  return QSysInfo::machineHostName();
}

QString userName() {
  const QByteArray user = qgetenv("USER");
  if (!user.isEmpty()) {
    return QString::fromLocal8Bit(user);
  }
  const QByteArray windowsUser = qgetenv("USERNAME");
  if (!windowsUser.isEmpty()) {
    return QString::fromLocal8Bit(windowsUser);
  }
  return {};
}

}  // namespace pfd::base
