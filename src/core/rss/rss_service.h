#ifndef PFD_CORE_RSS_RSS_SERVICE_H
#define PFD_CORE_RSS_RSS_SERVICE_H

#include <QtCore/QSet>
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

  [[nodiscard]] bool autoDownloadEnabled() const;
  void setAutoDownloadEnabled(bool enabled);
  void setMaxAutoDownloadsPerRefresh(int max);

  void setDownloadRequestCallback(DownloadRequestCallback cb);
  void setRequestHeaders(const RssFetcher::RequestHeaders& headers);
  void setHttpProxy(const RssFetcher::HttpProxyConfig& proxy);

  void upsertFeed(const RssFeed& feed);
  void removeFeed(const QString& feed_id);

  /// 删除属于该订阅源的所有本地条目（不删除订阅源本身）；重建去重索引。调用方应再 saveState。
  void clearItemsForFeed(const QString& feed_id);
  void clearItemsForFeeds(const QStringList& feed_ids);

  void upsertRule(const RssRule& rule);
  void removeRule(const QString& rule_id);

  void refreshAllFeeds();
  /// 先清空所有「已启用」订阅源下的本地条目，再执行 refreshAllFeeds（条目流「全部刷新」专用）。
  void reloadItemStreamFromEnabledFeeds();
  void refreshFeed(const QString& feed_id);

  void markItemRead(const QString& item_id);
  void markItemIgnored(const QString& item_id);
  void markItemsIgnored(const QStringList& item_ids);

  /// @return 是否已向下载回调投递请求（无资源或未注册回调时为 false）。
  [[nodiscard]] bool downloadItem(const QString& item_id);

  /// 由 AppController 在 RSS 磁力/种子流程成功或失败后调用。
  /// @param resolved_save_override 非空时写入条目的
  /// download_save_path（如用户在添加对话框中确认的目录）。
  void applyRssDownloadSettlement(const RssDownloadSettlement& settlement, bool success,
                                  const QString& resolved_save_override = {}, bool persist = true);

  [[nodiscard]] std::optional<RssFeed> findFeed(const QString& feed_id) const;
  [[nodiscard]] std::vector<RssItem> itemsForFeed(const QString& feed_id) const;
  [[nodiscard]] std::vector<RuleMatchResult> evaluateItem(const RssItem& item) const;

  void setHistoryPolicy(const HistoryPolicy& policy);
  int pruneHistory();

  [[nodiscard]] RssSettings settings() const;
  void applySettings(const RssSettings& s);

  /// 按当前全局/订阅源/规则设置，仅更新条目的自动下载诊断字段（不排队、不触发下载）。
  void refreshAutoDownloadDiagnostics();
  /// 仅重算属于 `feed_id` 的条目（如切换该源的「自动下载」后调用）。
  void refreshAutoDownloadDiagnosticsForFeed(const QString& feed_id);

  /// 在诊断与等待标记已更新的前提下，按「同时自动下载数」空槽逐条调用 `downloadItem`，每轮最多尝试
  /// `max_batch` 次成功派发。
  /// @param refresh_diagnosis 为 false 时跳过全量诊断（调用方刚执行过
  /// `refreshAutoDownloadDiagnostics` 时可省一次 O(n) 扫描）。
  /// @param scope `kAllEligible` 扫描全部符合且未下载条目；`kNewItemsOnly`
  /// 仅本轮网络刷新新入库条目。
  /// @return 成功发起（回调已投递）的条目数。
  [[nodiscard]] int
  dispatchDeferredAutoDownloads(int max_batch, bool refresh_diagnosis = true,
                                RssAutoDownloadScope scope = RssAutoDownloadScope::kAllEligible);

  [[nodiscard]] const QSet<QString>& lastRefreshNewItemIds() const;

private:
  void refreshFeedImpl(const QString& feed_id);

  void dedupIncoming(std::vector<RssItem>& incoming);
  void diagnoseItemAutoDownloadState(RssItem& item);
  void diagnoseRuleAutoDownloadState(RssItem& item);
  /// 删除 feed_id 非空但当前 feeds_ 中不存在的条目（合并订阅 id、手改数据等导致的孤儿）。
  void pruneItemsWithoutMatchingFeed();

  void syncRssAutoWaitlistMarkers(RssAutoDownloadScope scope);
  void rebuildAutoBacklogScratch(RssAutoDownloadScope scope);
  void applyAutoDownloadBacklogOverflowMarkers();
  [[nodiscard]] int effectiveAutoDownloadBacklogCap() const;
  [[nodiscard]] bool itemMatchesAutoDownloadScope(const RssItem& item,
                                                  RssAutoDownloadScope scope) const;

  RssRepository repository_;
  RssFetcher fetcher_;
  RssParser parser_;
  RssDedup dedup_;
  std::vector<RssFeed> feeds_;
  std::vector<RssItem> items_;
  std::vector<RssRule> rules_;
  bool auto_download_enabled_{false};
  int max_auto_per_refresh_{kAutoDownloadMaxPerRefresh};
  HistoryPolicy history_policy_;
  RssSettings settings_;
  DownloadRequestCallback on_download_request_;
  /// 复用缓冲：按 `published_at` 降序排列的 eligible 条目在 `items_` 中的下标（避免每次
  /// sync/dispatch 堆分配）。
  std::vector<std::size_t> auto_backlog_scratch_;
  /// 最近一次 `refreshAllFeeds` 周期内新入库的条目 id（`refreshFeed` 单独刷新时追加，不清空）。
  QSet<QString> last_refresh_new_item_ids_;
};

}  // namespace pfd::core::rss

#endif  // PFD_CORE_RSS_RSS_SERVICE_H
