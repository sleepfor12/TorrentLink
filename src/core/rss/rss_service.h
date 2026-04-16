#ifndef PFD_CORE_RSS_RSS_SERVICE_H
#define PFD_CORE_RSS_RSS_SERVICE_H

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <functional>
#include <optional>
#include <unordered_set>
#include <vector>

#include "core/rss/rss_dedup.h"
#include "core/rss/rss_fetcher.h"
#include "core/rss/rss_parser.h"
#include "core/rss/rss_repository.h"
#include "core/rss/rss_rule_engine.h"
#include "core/rss/rss_types.h"

namespace pfd::core::rss {

class RssService {
public:
  using DownloadRequestCallback = std::function<void(const AutoDownloadRequest&)>;

  explicit RssService(QString data_dir);

  void loadState();
  void saveState() const;

  [[nodiscard]] const std::vector<RssFeed>& feeds() const;
  [[nodiscard]] const std::vector<RssItem>& items() const;
  [[nodiscard]] const std::vector<RssRule>& rules() const;
  [[nodiscard]] const std::vector<SeriesSubscription>& series() const;

  [[nodiscard]] bool autoDownloadEnabled() const;
  void setAutoDownloadEnabled(bool enabled);
  void setMaxAutoDownloadsPerRefresh(int max);

  void setDownloadRequestCallback(DownloadRequestCallback cb);

  void upsertFeed(const RssFeed& feed);
  void removeFeed(const QString& feed_id);

  /// 删除属于该订阅源的所有本地条目（不删除订阅源本身）；重建去重索引。调用方应再 saveState。
  void clearItemsForFeed(const QString& feed_id);
  void clearItemsForFeeds(const QStringList& feed_ids);

  void upsertRule(const RssRule& rule);
  void removeRule(const QString& rule_id);

  void upsertSeries(const SeriesSubscription& entry);
  void removeSeries(const QString& series_id);

  void refreshAllFeeds();
  void refreshFeed(const QString& feed_id);

  void markItemRead(const QString& item_id);
  void markItemIgnored(const QString& item_id);
  void markItemsIgnored(const QStringList& item_ids);

  /// @return 是否已向下载回调投递请求（无资源或未注册回调时为 false）。
  [[nodiscard]] bool downloadItem(const QString& item_id);

  /// 由 AppController 在 RSS
  /// 磁力/种子流程成功或失败后调用；成功时标记条目已下载并可选推进剧集订阅。
  /// @param resolved_save_override 非空时写入条目的
  /// download_save_path（如用户在添加对话框中确认的目录）。
  void applyRssDownloadSettlement(const RssDownloadSettlement& settlement, bool success,
                                  const QString& resolved_save_override = {});

  [[nodiscard]] std::optional<RssFeed> findFeed(const QString& feed_id) const;
  [[nodiscard]] std::vector<RssItem> itemsForFeed(const QString& feed_id) const;
  [[nodiscard]] std::vector<RuleMatchResult> evaluateItem(const RssItem& item) const;

  [[nodiscard]] std::optional<EpisodeInfo> parseEpisode(const QString& title) const;
  [[nodiscard]] std::optional<SeriesSubscription> matchSeries(const RssItem& item) const;

  void setHistoryPolicy(const HistoryPolicy& policy);
  int pruneHistory();

  [[nodiscard]] RssSettings settings() const;
  void applySettings(const RssSettings& s);

private:
  void dedupIncoming(std::vector<RssItem>& incoming);
  bool tryAutoDownload(RssItem& item);
  bool trySeriesAutoDownload(RssItem& item);

  RssRepository repository_;
  RssFetcher fetcher_;
  RssParser parser_;
  RssDedup dedup_;
  std::vector<RssFeed> feeds_;
  std::vector<RssItem> items_;
  std::vector<RssRule> rules_;
  std::vector<SeriesSubscription> series_;
  bool auto_download_enabled_{false};
  int max_auto_per_refresh_{kAutoDownloadMaxPerRefresh};
  int auto_download_attempts_this_refresh_{0};
  HistoryPolicy history_policy_;
  RssSettings settings_;
  DownloadRequestCallback on_download_request_;
};

}  // namespace pfd::core::rss

#endif  // PFD_CORE_RSS_RSS_SERVICE_H
