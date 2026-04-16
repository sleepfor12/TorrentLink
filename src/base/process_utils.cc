#include "base/process_utils.h"

#include <QtCore/QProcess>

namespace pfd::base {

QString withPlatformExecutableSuffix(const QString& program) {
  const QString trimmed = program.trimmed();
  if (trimmed.isEmpty()) {
    return {};
  }
#ifdef _WIN32
  if (trimmed.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive) ||
      trimmed.endsWith(QStringLiteral(".bat"), Qt::CaseInsensitive) ||
      trimmed.endsWith(QStringLiteral(".cmd"), Qt::CaseInsensitive)) {
    return trimmed;
  }
  return trimmed + QStringLiteral(".exe");
#else
  return trimmed;
#endif
}

DetachedCommand buildDetachedCommand(const QString& commandLine, const QStringList& extraArgs) {
  DetachedCommand out;
  const QStringList tokens = QProcess::splitCommand(commandLine.trimmed());
  if (tokens.isEmpty()) {
    return out;
  }
  out.program = tokens.front().trimmed();
  out.arguments = tokens.mid(1);
  out.arguments.append(extraArgs);

#ifdef _WIN32
  // Windows 上常见用户仅填写 "vlc" 这类命令名，尝试补全为 .exe。
  if (!out.program.contains(QLatin1Char('/')) && !out.program.contains(QLatin1Char('\\'))) {
    out.program = withPlatformExecutableSuffix(out.program);
  }
#endif
  return out;
}

}  // namespace pfd::base
