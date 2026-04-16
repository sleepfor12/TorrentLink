#ifndef PFD_BASE_TYPES_H
#define PFD_BASE_TYPES_H

#include <QtCore/QString>
#include <QtCore/QStringView>
#include <QtCore/QUuid>

#include <cstdint>

namespace pfd::base {

/// 任务 ID 类型，供 Core↔UI 和 lt 层契约使用
using TaskId = QUuid;

/// 任务状态（lt 状态 → 此枚举，由 lt 层转换）
enum class TaskStatus : std::uint8_t {
  kUnknown = 0,
  kQueued,       // 排队中
  kChecking,     // 校验中
  kDownloading,  // 下载中
  kSeeding,      // 做种中
  kPaused,       // 已暂停
  kFinished,     // 已完成
  kError,        // 错误
};

/// 日志等级（用于 TaskLogEntry、LogStore）
enum class LogLevel : std::uint8_t {
  kDebug = 0,
  kInfo,
  kWarning,
  kError,
  kFatal,
};

/// 日志等级 → 小写英文字符串（用于日志输出/序列化）
[[nodiscard]] inline QStringView logLevelToString(LogLevel level) {
  switch (level) {
    case LogLevel::kDebug:
      return QStringView{u"debug"};
    case LogLevel::kInfo:
      return QStringView{u"info"};
    case LogLevel::kWarning:
      return QStringView{u"warning"};
    case LogLevel::kError:
      return QStringView{u"error"};
    case LogLevel::kFatal:
      return QStringView{u"fatal"};
  }
  return QStringView{u"unknown"};
}

/// 字符串（大小写不敏感）→ 日志等级；失败返回 false
[[nodiscard]] inline bool tryParseLogLevel(const QString& text, LogLevel* out) {
  if (out == nullptr) {
    return false;
  }
  const QString norm = text.trimmed().toLower();
  if (norm == QStringLiteral("debug")) {
    *out = LogLevel::kDebug;
    return true;
  }
  if (norm == QStringLiteral("info")) {
    *out = LogLevel::kInfo;
    return true;
  }
  if (norm == QStringLiteral("warning") || norm == QStringLiteral("warn")) {
    *out = LogLevel::kWarning;
    return true;
  }
  if (norm == QStringLiteral("error")) {
    *out = LogLevel::kError;
    return true;
  }
  if (norm == QStringLiteral("fatal")) {
    *out = LogLevel::kFatal;
    return true;
  }
  return false;
}

/// 是否完成（用于任务是否 100%）
[[nodiscard]] inline bool isFinished(TaskStatus s) {
  return s == TaskStatus::kFinished || s == TaskStatus::kSeeding;
}

/// 是否出错
[[nodiscard]] inline bool isError(TaskStatus s) {
  return s == TaskStatus::kError;
}

}  // namespace pfd::base

#endif  // PFD_BASE_TYPES_H
