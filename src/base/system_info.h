#ifndef PFD_BASE_SYSTEM_INFO_H
#define PFD_BASE_SYSTEM_INFO_H

#include <QtCore/QString>

namespace pfd::base {

[[nodiscard]] qint64 processId();
[[nodiscard]] QString hostName();
[[nodiscard]] QString userName();

}  // namespace pfd::base

#endif  // PFD_BASE_SYSTEM_INFO_H
