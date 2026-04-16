#ifndef PFD_LT_TASK_ALERT_ADAPTER_H
#define PFD_LT_TASK_ALERT_ADAPTER_H

#include <libtorrent/alert.hpp>
#include <libtorrent/torrent_status.hpp>

#include <optional>
#include <vector>

#include "lt/task_event_mapper.h"

namespace pfd::lt {

/// @brief 将真实 libtorrent alert 适配为 LtAlertView（无法识别时返回 nullopt）
[[nodiscard]] std::optional<LtAlertView> adaptAlertToView(const libtorrent::alert* alert);
[[nodiscard]] std::vector<LtAlertView> adaptAlertToViews(const libtorrent::alert* alert);
[[nodiscard]] LtAlertView
adaptTorrentStatusToProgressView(const libtorrent::torrent_status& status);

}  // namespace pfd::lt

#endif  // PFD_LT_TASK_ALERT_ADAPTER_H
