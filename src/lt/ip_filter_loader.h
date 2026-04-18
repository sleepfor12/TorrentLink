#ifndef PFD_LT_IP_FILTER_LOADER_H
#define PFD_LT_IP_FILTER_LOADER_H

#include <QtCore/QString>
#include <QtCore/QStringView>

#include <libtorrent/ip_filter.hpp>

namespace pfd::lt {

/// 从文本文件加载 IP 过滤规则到 @p out（在默认“全放行”之上叠加 blocked 段）。
/// 支持 IPv4 / IPv6：单行地址、CIDR、起始 - 结束；`#` 行注释与空行。
/// @return 成功打开并扫描文件则为 true；无法打开文件则为 false（@p error 含原因）。
[[nodiscard]] bool loadIpFilterFile(const QString& path, libtorrent::ip_filter* out,
                                    int* rules_applied, QString* error);

/// 解析单行（用于单元测试与复用）。
[[nodiscard]] bool appendIpFilterLine(QStringView line, libtorrent::ip_filter* out);

}  // namespace pfd::lt

#endif  // PFD_LT_IP_FILTER_LOADER_H
