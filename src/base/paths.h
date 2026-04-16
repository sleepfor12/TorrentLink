#ifndef PFD_BASE_PATHS_H
#define PFD_BASE_PATHS_H

#include <QtCore/QString>

namespace pfd::base {

/// 应用数据目录（如 ~/.local/share/p2p_downloader 或 Windows AppData 路径），尾含平台分隔符。
[[nodiscard]] QString appDataDir();

/// 配置目录（如 ~/.config/p2p_downloader 或 Windows AppData 配置路径），尾含平台分隔符。
[[nodiscard]] QString configDir();

/// resume 子目录（appDataDir + "resume"），尾含平台分隔符。
[[nodiscard]] QString resumeDir();

/// 递归创建目录（mkdir -p）；path 为空或无效返回 false
[[nodiscard]] bool ensureExists(const QString& path);

}  // namespace pfd::base

#endif  // PFD_BASE_PATHS_H
