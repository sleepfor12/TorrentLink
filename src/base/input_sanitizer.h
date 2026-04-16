#ifndef PFD_BASE_INPUT_SANITIZER_H
#define PFD_BASE_INPUT_SANITIZER_H

#include <QtCore/QString>
#include <QtCore/QStringList>

#include "base/errors.h"

namespace pfd::base {

// ─── 约束常量 ───

inline constexpr int kMaxMagnetUriLength = 8192;
inline constexpr int kMaxTrackerUrlLength = 2048;
inline constexpr int kMaxPathLength = 4096;
inline constexpr int kMaxTorrentFileSize = 100 * 1024 * 1024;  // 100 MiB
inline constexpr int kMaxTrackersPerTask = 200;
inline constexpr int kMaxCategoryLength = 256;
inline constexpr int kMaxTagLength = 128;
inline constexpr int kMaxTagsCount = 50;

// ─── 验证函数 ───
// 返回 Error：isOk() 表示通过，hasError() 表示拒绝并携带具体原因。

/// 验证磁力链接 URI：非空、长度上限、必须以 "magnet:?" 开头、无空字节。
[[nodiscard]] Error validateMagnetUri(const QString& uri);

/// 验证 Tracker URL：长度上限、协议白名单（http/https/udp/wss）、无空字节。
[[nodiscard]] Error validateTrackerUrl(const QString& url);

/// 批量验证 tracker 列表，同时检查数量上限。
/// 返回第一个失败的 Error；全部通过返回 isOk()。
[[nodiscard]] Error validateTrackerList(const QStringList& trackers);

/// 验证文件系统路径（保存路径/torrent 文件路径）：
/// 非空、长度上限、无空字节、无 ".." 路径穿越。
[[nodiscard]] Error validatePath(const QString& path);

/// 验证 .torrent 文件路径：在 validatePath 基础上检查扩展名。
[[nodiscard]] Error validateTorrentFilePath(const QString& path);

/// 验证类别名称。
[[nodiscard]] Error validateCategory(const QString& category);

/// 验证标签 CSV（逗号分隔）。
[[nodiscard]] Error validateTagsCsv(const QString& tagsCsv);

/// 批量清洗 tracker 列表：去空、去重、跳过不合法条目；返回干净列表。
[[nodiscard]] QStringList sanitizeTrackers(const QStringList& trackers);

}  // namespace pfd::base

#endif  // PFD_BASE_INPUT_SANITIZER_H
