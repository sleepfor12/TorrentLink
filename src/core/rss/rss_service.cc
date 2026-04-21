#include "core/rss/rss_service.h"

#include <QtCore/QDateTime>
#include <QtCore/QSet>
#include <QtCore/QUuid>

#include <algorithm>

#include "core/logger.h"
#include "core/rss/rss_series_tracker.h"

namespace pfd::core::rss {

RssService::RssService(QString data_dir) : repository_(std::move(data_dir)) {}

void RssService::loadState() {
  feeds_ = repository_.loadFeeds();
  items_ = repository_.loadItems();
  rules_ = repository_.loadRules();
  series_ = repository_.loadSeries();
  settings_ = repository_.loadSettings();
  applySettings(settings_);
  dedup_.buildIndex(items_);
  LOG_INFO(QStringLiteral("[rss] State loaded: feeds=%1 items=%2 rules=%3 series=%4")
               .arg(feeds_.size())
               .arg(items_.size())
               .arg(rules_.size())
               .arg(series_.size()));
}

void RssService::saveState() const {
  repository_.saveFeeds(feeds_);
  repository_.saveItems(items_);
  repository_.saveRules(rules_);
  repository_.saveSeries(series_);
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
const std::vector<SeriesSubscription>& RssService::series() const {
  return series_;
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

void RssService::upsertFeed(const RssFeed& feed) {
  auto it =
      std::find_if(feeds_.begin(), feeds_.end(), [&](const RssFeed& x) { return x.id == feed.id; });
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
    *it = feed;
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

void RssService::upsertSeries(const SeriesSubscription& entry) {
  auto it = std::find_if(series_.begin(), series_.end(),
                         [&](const SeriesSubscription& x) { return x.id == entry.id; });
  if (it == series_.end()) {
    SeriesSubscription copy = entry;
    if (copy.id.isEmpty())
      copy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    series_.push_back(std::move(copy));
  } else {
    *it = entry;
  }
}

void RssService::removeSeries(const QString& series_id) {
  series_.erase(std::remove_if(series_.begin(), series_.end(),
                               [&](const SeriesSubscription& x) { return x.id == series_id; }),
                series_.end());
}

void RssService::refreshAllFeeds() {
  auto_download_attempts_this_refresh_ = 0;
  for (auto& feed : feeds_) {
    if (feed.enabled) {
      refreshFeed(feed.id);
    }
  }
  pruneHistory();
}

void RssService::refreshFeed(const QString& feed_id) {
  auto fit =
      std::find_if(feeds_.begin(), feeds_.end(), [&](const RssFeed& x) { return x.id == feed_id; });
  if (fit == feeds_.end())
    return;

  LOG_INFO(QStringLiteral("[rss] Refreshing feed: %1 (%2)").arg(fit->title, fit->url));

  const auto fetchResult = fetcher_.fetch(fit->url);
  fit->last_refreshed_at = QDateTime::currentDateTime();

  if (!fetchResult.ok) {
    fit->last_error = fetchResult.error;
    LOG_WARN(QStringLiteral("[rss] Fetch failed for %1: %2").arg(fit->title, fetchResult.error));
    return;
  }
  fit->last_error.clear();

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
    dedup_.recordItem(item);
    items_.push_back(std::move(item));
    ++added;
    if (auto_download_attempts_this_refresh_ < max_auto_per_refresh_) {
      if (tryAutoDownload(items_.back()) || trySeriesAutoDownload(items_.back())) {
        // attempts counted inside the try* methods
      }
    }
  }

  LOG_INFO(QStringLiteral("[rss] Feed %1: +%2 new items, auto-dl attempts this cycle: %3/%4")
               .arg(fit->title)
               .arg(added)
               .arg(auto_download_attempts_this_refresh_)
               .arg(max_auto_per_refresh_));
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
                                            const QString& resolved_save_override) {
  if (!success || s.item_id.isEmpty()) {
    return;
  }
  const QString pathToStore =
      resolved_save_override.trimmed().isEmpty() ? s.record_save_path : resolved_save_override;
  bool touched = false;
  for (auto& it : items_) {
    if (it.id != s.item_id) {
      continue;
    }
    it.downloaded = true;
    it.download_save_path = pathToStore;
    touched = true;
    break;
  }
  if (!s.series_sub_id.isEmpty()) {
    for (auto& sub : series_) {
      if (sub.id != s.series_sub_id) {
        continue;
      }
      if (s.series_episode >= 0) {
        sub.last_episode_num = s.series_episode;
      }
      sub.last_episode =
          QStringLiteral("S%1E%2")
              .arg(s.series_season >= 0 ? s.series_season : 0, 2, 10, QLatin1Char('0'))
              .arg(s.series_episode >= 0 ? s.series_episode : 0, 2, 10, QLatin1Char('0'));
      touched = true;
      break;
    }
  }
  if (touched) {
    saveState();
  }
}

bool RssService::downloadItem(const QString& item_id) {
  for (auto& it : items_) {
    if (it.id != item_id)
      continue;
    if (it.magnet.isEmpty() && it.torrent_url.isEmpty()) {
      LOG_WARN(QStringLiteral("[rss] Cannot download item \"%1\": no magnet or torrent URL")
                   .arg(it.title));
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

std::optional<EpisodeInfo> RssService::parseEpisode(const QString& title) const {
  return RssSeriesTracker::parseTitle(title);
}

std::optional<SeriesSubscription> RssService::matchSeries(const RssItem& item) const {
  auto ep = RssSeriesTracker::parseTitle(item.title);
  if (!ep.has_value())
    return std::nullopt;

  for (const auto& sub : series_) {
    if (!sub.enabled)
      continue;
    if (!RssSeriesTracker::matchesSeries(*ep, sub))
      continue;
    if (!RssSeriesTracker::isNewerEpisode(*ep, sub))
      continue;
    return sub;
  }
  return std::nullopt;
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
  incoming.erase(std::remove_if(incoming.begin(), incoming.end(),
                                [&](const RssItem& it) { return dedup_.isDuplicate(it); }),
                 incoming.end());
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
  settings_.history_max_items = std::clamp(settings_.history_max_items, 100, 100000);
  settings_.history_max_age_days = std::clamp(settings_.history_max_age_days, 0, 3650);
  settings_.external_player_command = settings_.external_player_command.trimmed();
  auto_download_enabled_ = settings_.global_auto_download;
  max_auto_per_refresh_ = settings_.max_auto_downloads_per_refresh;
  history_policy_.max_items = settings_.history_max_items;
  history_policy_.max_age_days = settings_.history_max_age_days;
}

bool RssService::tryAutoDownload(RssItem& item) {
  if (!auto_download_enabled_) {
    LOG_DEBUG(QStringLiteral("[rss] Auto-download skipped(global off) item=%1").arg(item.id));
    return false;
  }
  if (item.magnet.isEmpty() && item.torrent_url.isEmpty()) {
    LOG_DEBUG(
        QStringLiteral("[rss] Auto-download skipped(no magnet/torrent URL) item=%1").arg(item.id));
    return false;
  }
  if (item.downloaded || item.ignored) {
    LOG_DEBUG(QStringLiteral("[rss] Auto-download skipped(state downloaded=%1 ignored=%2) item=%3")
                  .arg(item.downloaded ? QStringLiteral("true") : QStringLiteral("false"))
                  .arg(item.ignored ? QStringLiteral("true") : QStringLiteral("false"))
                  .arg(item.id));
    return false;
  }

  auto fit = std::find_if(feeds_.begin(), feeds_.end(),
                          [&](const RssFeed& x) { return x.id == item.feed_id; });
  if (fit == feeds_.end() || !fit->auto_download_enabled) {
    LOG_DEBUG(QStringLiteral("[rss] Auto-download skipped(feed disabled/missing) feed=%1 item=%2")
                  .arg(item.feed_id, item.id));
    return false;
  }

  auto match = RssRuleEngine::findFirstEnabledMatch(rules_, item);
  if (!match.has_value()) {
    LOG_DEBUG(QStringLiteral("[rss] Auto-download skipped(no rule match) item=%1").arg(item.id));
    return false;
  }

  auto rit = std::find_if(rules_.begin(), rules_.end(),
                          [&](const RssRule& r) { return r.id == match->rule_id; });
  if (rit == rules_.end()) {
    LOG_WARN(QStringLiteral("[rss] Auto-download matched missing rule rule_id=%1 item=%2")
                 .arg(match->rule_id, item.id));
    return false;
  }

  if (!on_download_request_) {
    LOG_WARN(QStringLiteral("[rss] Auto-download matched but no callback item=%1").arg(item.id));
    return false;
  }

  AutoDownloadRequest req;
  req.magnet = item.magnet;
  req.torrent_url = item.torrent_url;
  req.save_path = rit->save_path;
  req.category = rit->category;
  req.tags_csv = rit->tags_csv;
  req.rule_id = rit->id;
  req.item_id = item.id;
  req.feed_id = item.feed_id;
  req.item_title = item.title;
  req.referer_url = fit->url;
  req.rss_settlement.item_id = item.id;
  req.rss_settlement.record_save_path = rit->save_path;
  req.add_without_interactive_confirm = true;
  ++auto_download_attempts_this_refresh_;
  on_download_request_(req);
  LOG_INFO(QStringLiteral("[rss] Auto-download: \"%1\" rule=\"%2\" feed=%3 item=%4")
               .arg(item.title, match->rule_name, item.feed_id, item.id));
  return true;
}

bool RssService::trySeriesAutoDownload(RssItem& item) {
  if (!auto_download_enabled_)
    return false;
  if ((item.magnet.isEmpty() && item.torrent_url.isEmpty()) || item.downloaded || item.ignored) {
    return false;
  }

  auto ep = RssSeriesTracker::parseTitle(item.title);
  if (!ep.has_value()) {
    LOG_DEBUG(QStringLiteral("[rss] Series auto-download skipped(parse episode failed) item=%1")
                  .arg(item.id));
    return false;
  }

  auto fit = std::find_if(feeds_.begin(), feeds_.end(),
                          [&](const RssFeed& x) { return x.id == item.feed_id; });
  const QString referer = fit != feeds_.end() ? fit->url : QString();

  for (auto& sub : series_) {
    if (!sub.enabled || !sub.auto_download_enabled)
      continue;
    if (!RssSeriesTracker::matchesSeries(*ep, sub))
      continue;
    if (!RssSeriesTracker::isNewerEpisode(*ep, sub))
      continue;

    if (!sub.quality_keywords.isEmpty()) {
      bool qualityOk = false;
      for (const auto& q : sub.quality_keywords) {
        if (item.title.contains(q, Qt::CaseInsensitive)) {
          qualityOk = true;
          break;
        }
      }
      if (!qualityOk)
        continue;
    }

    if (on_download_request_) {
      AutoDownloadRequest req;
      req.magnet = item.magnet;
      req.torrent_url = item.torrent_url;
      req.save_path = sub.save_path;
      req.item_id = item.id;
      req.feed_id = item.feed_id;
      req.item_title = item.title;
      req.referer_url = referer;
      req.rss_settlement.item_id = item.id;
      req.rss_settlement.record_save_path = sub.save_path;
      req.rss_settlement.series_sub_id = sub.id;
      req.rss_settlement.series_episode = ep->episode;
      req.rss_settlement.series_season = ep->season >= 0 ? ep->season : 0;
      req.add_without_interactive_confirm = true;
      ++auto_download_attempts_this_refresh_;
      on_download_request_(req);
      LOG_INFO(QStringLiteral("[rss] Series auto-download: \"%1\" series=\"%2\" ep=%3 feed=%4")
                   .arg(item.title, sub.name)
                   .arg(ep->episode)
                   .arg(item.feed_id));
    }
    return true;
  }
  LOG_DEBUG(QStringLiteral("[rss] Series auto-download no match item=%1 title=%2")
                .arg(item.id, item.title.left(80)));
  return false;
}

}  // namespace pfd::core::rss
