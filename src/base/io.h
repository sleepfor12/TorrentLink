#ifndef PFD_BASE_IO_H
#define PFD_BASE_IO_H

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <utility>

#include "base/errors.h"

namespace pfd::base {

/// 读取整个文件；失败时 error.hasError()，data 始终为空
[[nodiscard]] std::pair<QByteArray, Error> readWholeFile(const QString& path);

/// 读取整个文件为 UTF-8 文本；失败时 error.hasError()
[[nodiscard]] std::pair<QString, Error> readWholeFileAsText(const QString& path);

/// 原子写入整个文件（临时文件 + rename）；失败返回 Error
[[nodiscard]] Error writeWholeFile(const QString& path, const QByteArray& data);

/// 原子写入整个文本文件；失败返回 Error
[[nodiscard]] Error writeWholeFile(const QString& path, const QString& text);

/// 原子写入 + 写前将旧文件备份为 path + ".bak"。
/// 同时保留 ".bak2" 二级备份以应对主备份同时损坏的极端情况。
[[nodiscard]] Error writeWholeFileWithBackup(const QString& path, const QByteArray& data);

/// 尝试读取主文件，若失败/为空则自动回退到 ".bak" / ".bak2"。
/// 返回 {data, actualPath, error}；actualPath 指示最终读取的路径。
struct RecoveryReadResult {
  QByteArray data;
  QString actualPath;
  Error error;
};
[[nodiscard]] RecoveryReadResult readWholeFileWithRecovery(const QString& path);

/// 计算 QByteArray 的 SHA-256 摘要（hex 小写字符串）
[[nodiscard]] QString sha256Hex(const QByteArray& data);

}  // namespace pfd::base

#endif  // PFD_BASE_IO_H
