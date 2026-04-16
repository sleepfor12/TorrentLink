#ifndef PFD_BASE_PROCESS_UTILS_H
#define PFD_BASE_PROCESS_UTILS_H

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace pfd::base {

struct DetachedCommand {
  QString program;
  QStringList arguments;

  [[nodiscard]] bool isValid() const {
    return !program.trimmed().isEmpty();
  }
};

[[nodiscard]] QString withPlatformExecutableSuffix(const QString& program);
[[nodiscard]] DetachedCommand buildDetachedCommand(const QString& commandLine,
                                                   const QStringList& extraArgs = {});

}  // namespace pfd::base

#endif  // PFD_BASE_PROCESS_UTILS_H
