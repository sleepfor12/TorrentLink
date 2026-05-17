#include "core/rss/rss_service.h"

#include <QtCore/QDateTime>
#include <QtCore/QScopeGuard>
#include <QtCore/QSet>
#include <QtCore/QUuid>

#include <algorithm>

#include "core/logger.h"

namespace pfd::core::rss {

namespace {

void markItemDecision(RssItem& item, AutoDownloadDecision decision, const QString& reasonCode,
                      const QString& reasonText) {
  item.last_auto_decision = decision;
  item.last_auto_reason_code = reasonCode;
  item.last_auto_reason_text = reasonText;
}

[[nodiscard]] bool isDiagnosticAutoEligible(const QString& code) {
  return code == QStringLiteral("diagnostic_rule_eligible");
}

}  // namespace

RssService::RssService(QString data_dir) : repository_(std::move(data_dir)) {}

void RssService::loadState() {
  feeds_ = repository_.loadFeeds();
  items_ = repository_.loadItems();
  rules_ = repository_.loadRules();
  settings_ = repository_.loadSettings();
  for (auto& item : items_) {
    // queued is process-local transient state; always reset on startup.
    item.queued = false;
    item.rss_auto_waitlisted = false;
  }
  applySettings(settings_);
  pruneItemsWithoutMatchingFeed();
  dedup_.buildIndex(items_);
  auto_backlog_scratch_.clear();
  auto_backlog_scratch_.shrink_to_fit();
  LOG_INFO(QStringLiteral("[rss] State loaded: feeds=%1 items=%2 rules=%3")
               .arg(feeds_.size())
               .arg(items_.size())
               .arg(rules_.size()));
}

void RssService::pruneItemsWithoutMatchingFeed() {
  QSet<QString> feed_ids;
  feed_ids.reserve(static_cast<int>(feeds_.size()) + 1);
  for (const auto& f : feeds_) {
    feed_ids.insert(f.id);
  }
  const bool have_feeds = !feeds_.empty();
  const std::size_t before = items_.size();
  items_.erase(std::remove_if(items_.begin(), items_.end(),
                              [&](const RssItem& x) {
                                if (x.feed_id.isEmpty()) {
                                  // 有订阅时仍无 feed_id 的条目无法归属来源，视为损坏数据
                                  return have_feeds;
                                }
                                return !feed_ids.contains(x.feed_id);
                              }),
               items_.end());
  if (items_.size() != before) {
    LOG_WARN(
        QStringLiteral("[rss] Pruned orphan items (feed_id has no feed) removed=%1 remaining=%2")
            .arg(static_cast<int>(before - items_.size()))
            .arg(items_.size()));
  }
}

void RssService::saveState() const {
  repository_.saveFeeds(feeds_);
  repository_.saveItems(items_);
  repository_.saveRules(rules_);
  repository_.saveSettings(settings_);
}

const std::vector<RssFeed>& RssService::feeds() const {
  return feeds_;
}
const std::vector<RssItem>& RssService::items() const {
  return items_;
}
const std::vector<RssRule>& RssService::rules() const {
  return rules_;
}

bool RssService::autoDownloadEnabled() const {
  return auto_download_enabled_;
}
void RssService::setAutoDownloadEnabled(bool enabled) {
  auto_download_enabled_ = enabled;
  settings_.global_auto_download = enabled;
}
void RssService::setMaxAutoDownloadsPerRefresh(int max) {
  max_auto_per_refresh_ = std::clamp(max, 1, 100);
  settings_.max_auto_downloads_per_refresh = max_auto_per_refresh_;
}
void RssService::setDownloadRequestCallback(DownloadRequestCallback cb) {
  on_download_request_ = std::move(cb);
}

void RssService::setRequestHeaders(const RssFetcher::RequestHeaders& headers) {
  fetcher_.setRequestHeaders(headers);
}

void RssService::setHttpProxy(const RssFetcher::HttpProxyConfig& proxy) {
  fetcher_.setHttpProxy(proxy);
}

void RssService::upsertFeed(const RssFeed& feed) {
  std::vector<RssFeed>::iterator it = feeds_.end();
  if (!feed.id.isEmpty()) {
    it = std::find_if(feeds_.begin(), feeds_.end(),
                      [&](const RssFeed& x) { return x.id == feed.id; });
  }
  if (it == feeds_.end() && !feed.url.isEmpty()) {
    it = std::find_if(feeds_.begin(), feeds_.end(),
                      [&](const RssFeed& x) { return x.url == feed.url; });
  }
  if (it == feeds_.end()) {
    RssFeed copy = feed;
    if (copy.id.isEmpty())
      copy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    feeds_.push_back(std::move(copy));
    LOG_INFO(QStringLiteral("[rss] Feed added id=%1 title=%2 enabled=%3")
                 .arg(feeds_.back().id, feeds_.back().title)
                 .arg(feeds_.back().enabled ? QStringLiteral("true") : QStringLiteral("false")));
  } else {
    // 按 URL 合并时，新传入的 feed 往往 id 为空；必须保留原 id，否则条目 feed_id 与刷新目标会错乱。
    const RssFeed previous = *it;
    const QString previousId = previous.id;
    *it = feed;
    if (it->id.isEmpty()) {
      it->id =
          !previousId.isEmpty() ? previousId : QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    // 仅 id 为空（按 URL 命中已有订阅）时，传入对象常为只有 url/title 的默认构造，会误把
    // auto_download 等刷回默认关。
    if (feed.id.isEmpty()) {
      it->enabled = previous.enabled;
      it->auto_download_enabled = previous.auto_download_enabled;
      it->last_refreshed_at = previous.last_refreshed_at;
      it->last_success_refreshed_at = previous.last_success_refreshed_at;
      it->last_error = previous.last_error;
    }
    LOG_INFO(QStringLiteral("[rss] Feed updated id=%1 title=%2 enabled=%3")
                 .arg(it->id, it->title)
                 .arg(it->enabled ? QStringLiteral("true") : QStringLiteral("false")));
  }
}

void RssService::removeFeed(const QString& feed_id) {
  const int feedBefore = static_cast<int>(feeds_.size());
  const int itemBefore = static_cast<int>(items_.size());
  feeds_.erase(std::remove_if(feeds_.begin(), feeds_.end(),
                              [&](const RssFeed& x) { return x.id == feed_id; }),
               feeds_.end());
  items_.erase(std::remove_if(items_.begin(), items_.end(),
                              [&](const RssItem& x) { return x.feed_id == feed_id; }),
               items_.end());
  pruneItemsWithoutMatchingFeed();
  dedup_.buildIndex(items_);
  LOG_INFO(QStringLiteral("[rss] Feed removed id=%1 feeds:%2->%3 items:%4->%5")
               .arg(feed_id)
               .arg(feedBefore)
               .arg(feeds_.size())
               .arg(itemBefore)
               .arg(items_.size()));
}

void RssService::clearItemsForFeed(const QString& feed_id) {
  const int before = static_cast<int>(items_.size());
  items_.erase(std::remove_if(items_.begin(), items_.end(),
                              [&](const RssItem& x) { return x.feed_id == feed_id; }),
               items_.end());
  dedup_.buildIndex(items_);
  LOG_INFO(QStringLiteral("[rss] Cleared items for feed=%1 removed=%2 remaining_items=%3")
               .arg(feed_id)
               .arg(before - static_cast<int>(items_.size()))
               .arg(items_.size()));
}

void RssService::clearItemsForFeeds(const QStringList& feed_ids) {
  if (feed_ids.isEmpty())
    return;
  const QSet<QString> idset(feed_ids.begin(), feed_ids.end());
  const int before = static_cast<int>(items_.size());
  items_.erase(std::remove_if(items_.begin(), items_.end(),
                              [&](const RssItem& x) { return idset.contains(x.feed_id); }),
               items_.end());
  dedup_.buildIndex(items_);
  LOG_INFO(QStringLiteral("[rss] Cleared items for %1 feeds removed=%2 remaining_items=%3")
               .arg(feed_ids.size())
               .arg(before - static_cast<int>(items_.size()))
               .arg(items_.size()));
}

void RssService::upsertRule(const RssRule& rule) {
  auto it =
      std::find_if(rules_.begin(), rules_.end(), [&](const RssRule& x) { return x.id == rule.id; });
  if (it == rules_.end()) {
    RssRule copy = rule;
    if (copy.id.isEmpty())
      copy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    rules_.push_back(std::move(copy));
  } else {
    *it = rule;
  }
}

void RssService::removeRule(const QString& rule_id) {
  rules_.erase(std::remove_if(rules_.begin(), rules_.end(),
                              [&](const RssRule& x) { return x.id == rule_id; }),
               rules_.end());
}

void RssService::refreshAllFeeds() {
  last_refresh_new_item_ids_.clear();
  for (auto& feed : feeds_) {
    if (feed.enabled) {
      refreshFeedImpl(feed.id);
    }
  }
  pruneHistory();
  pruneItemsWithoutMatchingFeed();
  dedup_.buildIndex(items_);
}

void RssService::reloadItemStreamFromEnabledFeeds() {
  QSet<QString> enabled_ids;
  enabled_ids.reserve(static_cast<int>(feeds_.size()) + 1);
  for (const auto& f : feeds_) {
    if (f.enabled) {
      enabled_ids.insert(f.id);
    }
  }
  if (!enabled_ids.isEmpty()) {
    const int before = static_cast<int>(items_.size());
    items_.erase(std::remove_if(items_.begin(), items_.end(),
                                [&](const RssItem& x) { return enabled_ids.contains(x.feed_id); }),
                 items_.end());
    dedup_.buildIndex(items_);
    LOG_INFO(
        QStringLiteral(
            "[rss] Item stream reload: cleared items for %1 enabled feeds removed=%2 remaining=%3")
            .arg(enabled_ids.size())
            .arg(before - static_cast<int>(items_.size()))
            .arg(items_.size()));
  }
  refreshAllFeeds();
}

void RssService::refreshFeed(const QString& feed_id) {
  auto fit =
      std::find_if(feeds_.begin(), feeds_.end(), [&](const RssFeed& x) { return x.id == feed_id; });
  if (fit == feeds_.end())
    return;
  refreshFeedImpl(feed_id);
}

void RssService::refreshFeedImpl(const QString& feed_id) {
  auto fit =
      std::find_if(feeds_.begin(), feeds_.end(), [&](const RssFeed& x) { return x.id == feed_id; });
  if (fit == feeds_.end())
    return;

  const QScopeGuard on_exit([this]() {
    pruneItemsWithoutMatchingFeed();
    dedup_.buildIndex(items_);
  });

  LOG_INFO(QStringLiteral("[rss] Refreshing feed: %1 (%2)").arg(fit->title, fit->url));

  const auto fetchResult = fetcher_.fetch(fit->url);
  fit->last_refreshed_at = QDateTime::currentDateTime();

  if (!fetchResult.ok) {
    fit->last_error = fetchResult.error;
    LOG_WARN(QStringLiteral("[rss] Fetch failed for %1: %2").arg(fit->title, fetchResult.error));
    return;
  }
  fit->last_error.clear();
  fit->last_success_refreshed_at = fit->last_refreshed_at;

  auto parseResult = parser_.parse(feed_id, fetchResult.body);
  if (!parseResult.ok) {
    fit->last_error = parseResult.error;
    LOG_WARN(QStringLiteral("[rss] Parse failed for %1: %2").arg(fit->title, parseResult.error));
    return;
  }

  if (fit->title.isEmpty() && !parseResult.items.empty()) {
    fit->title = QStringLiteral("Feed (%1 items)").arg(parseResult.items.size());
  }

  dedupIncoming(parseResult.items);

  int added = 0;
  for (auto& item : parseResult.items) {
    // dedupIncoming 已对保留项调用 recordItem，含同批次内去重。
    last_refresh_new_item_ids_.insert(item.id);
    items_.push_back(std::move(item));
    ++added;
  }

  LOG_INFO(QStringLiteral("[rss] Feed %1: +%2 new items").arg(fit->title).arg(added));
}

void RssService::markItemRead(const QString& item_id) {
  for (auto& it : items_) {
    if (it.id == item_id) {
      it.read = true;
      return;
    }
  }
}

void RssService::markItemIgnored(const QString& item_id) {
  for (auto& it : items_) {
    if (it.id == item_id) {
      it.ignored = true;
      return;
    }
  }
}

void RssService::markItemsIgnored(const QStringList& item_ids) {
  if (item_ids.isEmpty())
    return;
  const QSet<QString> idset(item_ids.begin(), item_ids.end());
  for (auto& it : items_) {
    if (idset.contains(it.id))
      it.ignored = true;
  }
}

void RssService::applyRssDownloadSettlement(const RssDownloadSettlement& s, bool success,
                                            const QString& resolved_save_override, bool persist) {
  if (s.item_id.isEmpty()) {
    return;
  }
  const QString pathToStore =
      resolved_save_override.trimmed().isEmpty() ? s.record_save_path : resolved_save_override;
  bool touched = false;
  for (auto& it : items_) {
    if (it.id != s.item_id) {
      continue;
    }
    it.queued = false;
    it.last_attempt_at = QDateTime::currentDateTime();
    if (success) {
      it.accepted = true;
      it.downloaded = true;
      it.retry_count = 0;
      it.download_save_path = pathToStore;
      it.last_success_at = it.last_attempt_at;
      markItemDecision(it, AutoDownloadDecision::kSucceeded, QStringLiteral("settlement_success"),
                       QStringLiteral("Request accepted by download pipeline."));
    } else {
      ++it.retry_count;
      markItemDecision(it, AutoDownloadDecision::kFailed, QStringLiteral("settlement_failed"),
                       QStringLiteral("Request failed or cancelled."));
    }
    touched = true;
    break;
  }
  if (touched && persist) {
    saveState();
  }
}

bool RssService::downloadItem(const QString& item_id) {
  for (auto& it : items_) {
    if (it.id != item_id)
      continue;
    if (it.magnet.isEmpty() && it.torrent_url.isEmpty()) {
      markItemDecision(it, AutoDownloadDecision::kSkipped, QStringLiteral("no_resource"),
                       QStringLiteral("No magnet or torrent URL."));
      LOG_WARN(QStringLiteral("[rss] Cannot download item \"%1\": no magnet or torrent URL")
                   .arg(it.title));
      return false;
    }
    if (it.queued) {
      markItemDecision(it, AutoDownloadDecision::kSkipped, QStringLiteral("already_queued"),
                       QStringLiteral("Item is already queued."));
      return false;
    }
    QString referer;
    auto fit = std::find_if(feeds_.begin(), feeds_.end(),
                            [&](const RssFeed& x) { return x.id == it.feed_id; });
    if (fit != feeds_.end()) {
      referer = fit->url;
    }

    QString savePath;
    QString category;
    QString tagsCsv;
    if (auto match = RssRuleEngine::findFirstEnabledMatch(rules_, it); match.has_value()) {
      auto rit = std::find_if(rules_.begin(), rules_.end(),
                              [&](const RssRule& r) { return r.id == match->rule_id; });
      if (rit != rules_.end()) {
        savePath = rit->save_path;
        category = rit->category;
        tagsCsv = rit->tags_csv;
      }
    }

    if (!on_download_request_) {
      markItemDecision(it, AutoDownloadDecision::kFailed, QStringLiteral("no_callback"),
                       QStringLiteral("Download callback is not set."));
      return false;
    }
    AutoDownloadRequest req;
    req.magnet = it.magnet;
    req.torrent_url = it.torrent_url;
    req.save_path = savePath;
    req.category = category;
    req.tags_csv = tagsCsv;
    req.item_id = it.id;
    req.feed_id = it.feed_id;
    req.item_title = it.title;
    req.referer_url = referer;
    req.rss_settlement.item_id = it.id;
    req.rss_settlement.record_save_path = savePath;
    req.add_without_interactive_confirm = true;
    it.queued = true;
    it.last_attempt_at = QDateTime::currentDateTime();
    markItemDecision(it, AutoDownloadDecision::kQueued, QStringLiteral("manual_queued"),
                     QStringLiteral("Manual download queued."));
    on_download_request_(req);
    LOG_INFO(QStringLiteral("[rss] Manual download: \"%1\"").arg(it.title));
    return true;
  }
  return false;
}

std::optional<RssFeed> RssService::findFeed(const QString& feed_id) const {
  auto it =
      std::find_if(feeds_.begin(), feeds_.end(), [&](const RssFeed& x) { return x.id == feed_id; });
  return it != feeds_.end() ? std::optional<RssFeed>(*it) : std::nullopt;
}

std::vector<RssItem> RssService::itemsForFeed(const QString& feed_id) const {
  std::vector<RssItem> out;
  for (const auto& it : items_) {
    if (it.feed_id == feed_id)
      out.push_back(it);
  }
  return out;
}

std::vector<RuleMatchResult> RssService::evaluateItem(const RssItem& item) const {
  return RssRuleEngine::evaluateAll(rules_, item);
}

void RssService::setHistoryPolicy(const HistoryPolicy& policy) {
  history_policy_ = policy;
}

int RssService::pruneHistory() {
  const int removed = RssDedup::pruneHistory(items_, history_policy_);
  if (removed > 0) {
    dedup_.buildIndex(items_);
    LOG_INFO(
        QStringLiteral("[rss] Pruned %1 old items, remaining: %2").arg(removed).arg(items_.size()));
  }
  return removed;
}

void RssService::dedupIncoming(std::vector<RssItem>& incoming) {
  const int before = static_cast<int>(incoming.size());
  std::vector<RssItem> unique;
  unique.reserve(incoming.size());
  for (auto& it : incoming) {
    if (dedup_.isDuplicate(it)) {
      continue;
    }
    dedup_.recordItem(it);
    unique.push_back(std::move(it));
  }
  incoming = std::move(unique);
  const int after = static_cast<int>(incoming.size());
  if (before != after) {
    LOG_INFO(
        QStringLiteral("[rss] Dedup incoming dropped=%1 kept=%2").arg(before - after).arg(after));
  } else {
    LOG_DEBUG(QStringLiteral("[rss] Dedup incoming no duplicates count=%1").arg(after));
  }
}

RssSettings RssService::settings() const {
  return settings_;
}

void RssService::applySettings(const RssSettings& s) {
  settings_ = s;
  settings_.refresh_interval_minutes = std::clamp(settings_.refresh_interval_minutes, 5, 1440);
  settings_.max_auto_downloads_per_refresh =
      std::clamp(settings_.max_auto_downloads_per_refresh, 1, 100);
  settings_.max_concurrent_auto_downloads =
      std::clamp(settings_.max_concurrent_auto_downloads, 1, 50);
  settings_.max_auto_download_backlog = std::clamp(settings_.max_auto_download_backlog, 1, 500);
  settings_.history_max_items = std::clamp(settings_.history_max_items, 100, 100000);
  settings_.history_max_age_days = std::clamp(settings_.history_max_age_days, 0, 3650);
  settings_.external_player_command = settings_.external_player_command.trimmed();
  auto_download_enabled_ = settings_.global_auto_download;
  max_auto_per_refresh_ = settings_.max_auto_downloads_per_refresh;
  history_policy_.max_items = settings_.history_max_items;
  history_policy_.max_age_days = settings_.history_max_age_days;
}

const QSet<QString>& RssService::lastRefreshNewItemIds() const {
  return last_refresh_new_item_ids_;
}

int RssService::effectiveAutoDownloadBacklogCap() const {
  return std::clamp(settings_.max_auto_download_backlog, 1, 500);
}

void RssService::applyAutoDownloadBacklogOverflowMarkers() {
  if (!auto_download_enabled_) {
    return;
  }
  rebuildAutoBacklogScratch(RssAutoDownloadScope::kAllEligible);
  QSet<QString> tracked;
  tracked.reserve(static_cast<int>(auto_backlog_scratch_.size()) + 1);
  for (const std::size_t idx : auto_backlog_scratch_) {
    tracked.insert(items_[idx].id);
  }
  for (auto& it : items_) {
    if (it.queued || it.downloaded || it.ignored) {
      continue;
    }
    if (!isDiagnosticAutoEligible(it.last_auto_reason_code)) {
      continue;
    }
    if (tracked.contains(it.id)) {
      continue;
    }
    markItemDecision(it, AutoDownloadDecision::kSkipped, QStringLiteral("auto_backlog_overflow"),
                     QStringLiteral("符合条件但超出自动下载排队上限，将随进度陆续入队。"));
  }
}

void RssService::refreshAutoDownloadDiagnostics() {
  for (auto& it : items_) {
    diagnoseItemAutoDownloadState(it);
  }
  applyAutoDownloadBacklogOverflowMarkers();
  syncRssAutoWaitlistMarkers(RssAutoDownloadScope::kAllEligible);
}

void RssService::refreshAutoDownloadDiagnosticsForFeed(const QString& feed_id) {
  if (feed_id.isEmpty()) {
    return;
  }
  for (auto& it : items_) {
    if (it.feed_id == feed_id) {
      diagnoseItemAutoDownloadState(it);
    }
  }
  applyAutoDownloadBacklogOverflowMarkers();
  syncRssAutoWaitlistMarkers(RssAutoDownloadScope::kAllEligible);
}

bool RssService::itemMatchesAutoDownloadScope(const RssItem& item,
                                              RssAutoDownloadScope scope) const {
  if (scope == RssAutoDownloadScope::kNewItemsOnly) {
    return last_refresh_new_item_ids_.contains(item.id);
  }
  return true;
}

void RssService::rebuildAutoBacklogScratch(RssAutoDownloadScope scope) {
  auto_backlog_scratch_.clear();
  const std::size_t n = items_.size();
  if (n == 0) {
    return;
  }
  auto_backlog_scratch_.reserve(n / 8 + 1);
  for (std::size_t i = 0; i < n; ++i) {
    const RssItem& it = items_[i];
    if (!itemMatchesAutoDownloadScope(it, scope)) {
      continue;
    }
    if (it.queued || it.downloaded || it.ignored) {
      continue;
    }
    if (it.magnet.isEmpty() && it.torrent_url.isEmpty()) {
      continue;
    }
    if (!isDiagnosticAutoEligible(it.last_auto_reason_code)) {
      continue;
    }
    auto_backlog_scratch_.push_back(i);
  }
  const auto publishedDesc = [this](std::size_t ia, std::size_t ib) {
    const RssItem& a = items_[ia];
    const RssItem& b = items_[ib];
    const QDateTime ta =
        a.published_at.isValid() ? a.published_at : QDateTime::fromSecsSinceEpoch(0);
    const QDateTime tb =
        b.published_at.isValid() ? b.published_at : QDateTime::fromSecsSinceEpoch(0);
    return ta > tb;
  };
  const int backlogCap = effectiveAutoDownloadBacklogCap();
  if (static_cast<int>(auto_backlog_scratch_.size()) <= backlogCap) {
    std::sort(auto_backlog_scratch_.begin(), auto_backlog_scratch_.end(), publishedDesc);
  } else {
    std::partial_sort(auto_backlog_scratch_.begin(), auto_backlog_scratch_.begin() + backlogCap,
                      auto_backlog_scratch_.end(), publishedDesc);
    auto_backlog_scratch_.resize(static_cast<std::size_t>(backlogCap));
  }
}

void RssService::syncRssAutoWaitlistMarkers(RssAutoDownloadScope scope) {
  int inflight = 0;
  for (auto& it : items_) {
    if (scope == RssAutoDownloadScope::kAllEligible || itemMatchesAutoDownloadScope(it, scope)) {
      it.rss_auto_waitlisted = false;
    }
    if (it.queued) {
      ++inflight;
    }
  }
  if (!auto_download_enabled_) {
    auto_backlog_scratch_.clear();
    return;
  }
  rebuildAutoBacklogScratch(scope);
  const int maxConc = std::clamp(settings_.max_concurrent_auto_downloads, 1, 50);
  const int freeSlots = std::max(0, maxConc - inflight);
  for (int i = freeSlots; i < static_cast<int>(auto_backlog_scratch_.size()); ++i) {
    items_[auto_backlog_scratch_[static_cast<std::size_t>(i)]].rss_auto_waitlisted = true;
  }
}

int RssService::dispatchDeferredAutoDownloads(int max_batch, bool refresh_diagnosis,
                                              RssAutoDownloadScope scope) {
  if (!auto_download_enabled_) {
    return 0;
  }
  const int cap = std::clamp(max_batch, 1, 100);
  const int maxConc = std::clamp(settings_.max_concurrent_auto_downloads, 1, 50);

  if (refresh_diagnosis) {
    for (auto& it : items_) {
      diagnoseItemAutoDownloadState(it);
    }
  }

  int inflight = 0;
  for (const auto& it : items_) {
    if (it.queued) {
      ++inflight;
    }
  }

  rebuildAutoBacklogScratch(scope);
  std::size_t scratchPos = 0;
  int started = 0;
  while (started < cap && inflight < maxConc && scratchPos < auto_backlog_scratch_.size()) {
    RssItem& it = items_[auto_backlog_scratch_[scratchPos++]];
    if (it.queued || it.downloaded || it.ignored) {
      continue;
    }
    if (!isDiagnosticAutoEligible(it.last_auto_reason_code)) {
      continue;
    }
    if (!downloadItem(it.id)) {
      // 单条失败（如回调暂不可用）不阻塞其余候选；连续失败由 cap / 空槽终止。
      continue;
    }
    ++started;
    ++inflight;
  }

  applyAutoDownloadBacklogOverflowMarkers();
  syncRssAutoWaitlistMarkers(scope);
  return started;
}

void RssService::diagnoseItemAutoDownloadState(RssItem& item) {
  if (!auto_download_enabled_) {
    markItemDecision(item, AutoDownloadDecision::kSkipped, QStringLiteral("global_disabled"),
                     QStringLiteral("Global auto download is disabled."));
    return;
  }
  if (item.magnet.isEmpty() && item.torrent_url.isEmpty()) {
    markItemDecision(item, AutoDownloadDecision::kSkipped, QStringLiteral("no_resource"),
                     QStringLiteral("No magnet or torrent URL."));
    return;
  }
  if (item.downloaded || item.ignored || item.queued) {
    markItemDecision(item, AutoDownloadDecision::kSkipped, QStringLiteral("state_blocked"),
                     QStringLiteral("Item is downloaded/ignored/queued."));
    return;
  }
  diagnoseRuleAutoDownloadState(item);
}

void RssService::diagnoseRuleAutoDownloadState(RssItem& item) {
  auto fit = std::find_if(feeds_.begin(), feeds_.end(),
                          [&](const RssFeed& x) { return x.id == item.feed_id; });
  if (fit == feeds_.end()) {
    markItemDecision(
        item, AutoDownloadDecision::kSkipped, QStringLiteral("unknown_feed"),
        QStringLiteral("条目的 feed_id 与当前订阅列表不一致（可能为历史数据或合并订阅导致）。"));
    return;
  }
  if (!fit->auto_download_enabled) {
    markItemDecision(item, AutoDownloadDecision::kSkipped, QStringLiteral("feed_auto_off"),
                     QStringLiteral("该订阅源已关闭「自动下载」。"));
    return;
  }

  auto match = RssRuleEngine::findFirstEnabledMatch(rules_, item);
  if (!match.has_value()) {
    markItemDecision(item, AutoDownloadDecision::kSkipped, QStringLiteral("no_rule_match"),
                     QStringLiteral("No rule matched."));
    return;
  }

  auto rit = std::find_if(rules_.begin(), rules_.end(),
                          [&](const RssRule& r) { return r.id == match->rule_id; });
  if (rit == rules_.end()) {
    markItemDecision(item, AutoDownloadDecision::kFailed, QStringLiteral("rule_missing"),
                     QStringLiteral("Matched rule not found."));
    return;
  }
  if (!on_download_request_) {
    markItemDecision(item, AutoDownloadDecision::kFailed, QStringLiteral("no_callback"),
                     QStringLiteral("Download callback is not set."));
    return;
  }
  markItemDecision(item, AutoDownloadDecision::kSkipped, QStringLiteral("diagnostic_rule_eligible"),
                   QStringLiteral("Rule would auto-download (diagnostic only)."));
}

}  // namespace pfd::core::rss
