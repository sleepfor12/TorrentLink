#ifndef PFD_CORE_RSS_RSS_TYPES_H
#define PFD_CORE_RSS_RSS_TYPES_H

#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <vector>

namespace pfd::core::rss {

struct RssFeed {
  QString id;
  QString title;
  QString url;
  bool enabled{true};
  bool auto_download_enabled{false};
  QDateTime last_refreshed_at;
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

struct SeriesSubscription {
  QString id;
  QString name;
  QStringList aliases;
  QStringList quality_keywords;
  bool enabled{true};
  bool auto_download_enabled{false};
  int season{-1};
  int last_episode_num{0};
  QString last_episode;
  QString save_path;
};

struct RuleMatchResult {
  QString rule_id;
  QString rule_name;
  bool matched{false};
  QString reason;
};

struct EpisodeInfo {
  QString series_name;
  int season{-1};
  int episode{-1};
  QString quality;
  QString raw_title;
};

/// 非空 item_id 时由 AppController 在下载流程结束时回调 applyRssDownloadSettlement。
struct RssDownloadSettlement {
  QString item_id;
  QString record_save_path;
  QString series_sub_id;
  int series_episode{-1};
  int series_season{-1};
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
  /// 为 true 时由 AppController 在元数据就绪后直接加入会话，不弹出「添加任务」对话框（RSS 手动/自动下载）。
  bool add_without_interactive_confirm{false};
};

inline constexpr int kAutoDownloadMaxPerRefresh = 10;

struct RssSettings {
  bool global_auto_download{false};
  int refresh_interval_minutes{30};
  int max_auto_downloads_per_refresh{kAutoDownloadMaxPerRefresh};
  int history_max_items{5000};
  int history_max_age_days{90};
  QString external_player_command;
};

}  // namespace pfd::core::rss

#endif  // PFD_CORE_RSS_RSS_TYPES_H
