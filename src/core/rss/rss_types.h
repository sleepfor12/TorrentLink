#ifndef PFD_CORE_RSS_RSS_TYPES_H
#define PFD_CORE_RSS_RSS_TYPES_H

#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace pfd::core::rss {

enum class AutoDownloadDecision : int {
  kUnknown = 0,
  kQueued,
  kSkipped,
  kSucceeded,
  kFailed,
};

struct RssFeed {
  QString id;
  QString title;
  QString url;
  bool enabled{true};
  bool auto_download_enabled{false};
  QDateTime last_refreshed_at;
  QDateTime last_success_refreshed_at;
  QString last_error;
};

struct RssItem {
  QString id;
  QString feed_id;
  QString title;
  QString link;
  QString guid;
  QString summary;
  QDateTime published_at;
  QString magnet;
  QString torrent_url;
  bool read{false};
  bool ignored{false};
  bool downloaded{false};
  bool accepted{false};
  bool queued{false};
  /// 仅内存态（不落盘）：满足自动下载条件但因「同时自动下载数」已满而尚未投递到传输队列。
  bool rss_auto_waitlisted{false};
  AutoDownloadDecision last_auto_decision{AutoDownloadDecision::kUnknown};
  QString last_auto_reason_code;
  QString last_auto_reason_text;
  QDateTime last_attempt_at;
  QDateTime last_success_at;
  int retry_count{0};
  QString download_save_path;
};

struct RssRule {
  QString id;
  QString name;
  QStringList feed_ids;
  QStringList include_keywords;
  QStringList exclude_keywords;
  bool use_regex{false};
  bool enabled{false};
  QString save_path;
  QString category;
  QString tags_csv;
};

struct RuleMatchResult {
  QString rule_id;
  QString rule_name;
  bool matched{false};
  QString reason;
};

/// 非空 item_id 时由 AppController 在下载流程结束时回调 applyRssDownloadSettlement。
struct RssDownloadSettlement {
  QString item_id;
  QString record_save_path;
};

struct AutoDownloadRequest {
  QString magnet;
  QString torrent_url;
  QString save_path;
  QString category;
  QString tags_csv;
  QString rule_id;
  QString item_id;
  QString feed_id;
  QString item_title;
  /// 拉取 HTTP .torrent 时作为 Referer（通常为订阅源 feed URL）。
  QString referer_url;
  RssDownloadSettlement rss_settlement;
  /// 为 true 时由 AppController 在元数据就绪后直接加入会话，不弹出「添加任务」对话框（RSS
  /// 手动/自动下载）。
  bool add_without_interactive_confirm{false};
};

inline constexpr int kAutoDownloadMaxPerRefresh = 10;
inline constexpr int kRssDefaultConcurrentAutoDownloads = 3;
inline constexpr int kRssDefaultAutoDownloadBacklog = 50;
inline constexpr int kRssDefaultRefreshIntervalMinutes = 30;

/// 自动下载候选范围：`kAllEligible` 为按诊断可派发的候选（受批量/并发/排队上限约束）；
/// `kNewItemsOnly`
/// 仅本轮刷新新入库条目（测试或专用增量逻辑；定时器勿单独依赖以免无新条目时不派发）。
enum class RssAutoDownloadScope {
  kAllEligible = 0,
  kNewItemsOnly,
};

struct RssSettings {
  bool global_auto_download{false};
  int refresh_interval_minutes{kRssDefaultRefreshIntervalMinutes};
  int max_auto_downloads_per_refresh{kAutoDownloadMaxPerRefresh};
  /// 与「下载选中」相同路径；限制同时投递到传输流程的 RSS 条目数（见
  /// `RssItem::queued`），超出部分标记 `rss_auto_waitlisted` 并在有空槽时继续派发。
  int max_concurrent_auto_downloads{kRssDefaultConcurrentAutoDownloads};
  /// 同时跟踪/等待自动入队的最大候选条目数（按发布时间优先），超出者不进入排队 scratch。
  int max_auto_download_backlog{kRssDefaultAutoDownloadBacklog};
  int history_max_items{5000};
  int history_max_age_days{90};
  QString external_player_command;
};

}  // namespace pfd::core::rss

#endif  // PFD_CORE_RSS_RSS_TYPES_H
