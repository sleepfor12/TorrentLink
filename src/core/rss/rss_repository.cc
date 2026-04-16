#include "core/rss/rss_repository.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <algorithm>

#include "base/io.h"
#include "core/logger.h"

namespace pfd::core::rss {

namespace {

constexpr int kRssJsonVersion = 1;

// ─── Feed serialization ───

QJsonObject feedToJson(const RssFeed& f) {
  QJsonObject o;
  o[QStringLiteral("id")] = f.id;
  o[QStringLiteral("title")] = f.title;
  o[QStringLiteral("url")] = f.url;
  o[QStringLiteral("enabled")] = f.enabled;
  o[QStringLiteral("auto_download_enabled")] = f.auto_download_enabled;
  o[QStringLiteral("last_refreshed_at")] = f.last_refreshed_at.toString(Qt::ISODate);
  o[QStringLiteral("last_error")] = f.last_error;
  return o;
}

RssFeed feedFromJson(const QJsonObject& o) {
  RssFeed f;
  f.id = o[QStringLiteral("id")].toString();
  f.title = o[QStringLiteral("title")].toString();
  f.url = o[QStringLiteral("url")].toString();
  f.enabled = o[QStringLiteral("enabled")].toBool(true);
  f.auto_download_enabled = o[QStringLiteral("auto_download_enabled")].toBool(false);
  f.last_refreshed_at =
      QDateTime::fromString(o[QStringLiteral("last_refreshed_at")].toString(), Qt::ISODate);
  f.last_error = o[QStringLiteral("last_error")].toString();
  return f;
}

// ─── Item serialization ───

QJsonObject itemToJson(const RssItem& it) {
  QJsonObject o;
  o[QStringLiteral("id")] = it.id;
  o[QStringLiteral("feed_id")] = it.feed_id;
  o[QStringLiteral("title")] = it.title;
  o[QStringLiteral("link")] = it.link;
  o[QStringLiteral("guid")] = it.guid;
  o[QStringLiteral("summary")] = it.summary;
  o[QStringLiteral("published_at")] = it.published_at.toString(Qt::ISODate);
  o[QStringLiteral("magnet")] = it.magnet;
  o[QStringLiteral("torrent_url")] = it.torrent_url;
  o[QStringLiteral("read")] = it.read;
  o[QStringLiteral("ignored")] = it.ignored;
  o[QStringLiteral("downloaded")] = it.downloaded;
  if (!it.download_save_path.isEmpty())
    o[QStringLiteral("download_save_path")] = it.download_save_path;
  return o;
}

RssItem itemFromJson(const QJsonObject& o) {
  RssItem it;
  it.id = o[QStringLiteral("id")].toString();
  it.feed_id = o[QStringLiteral("feed_id")].toString();
  it.title = o[QStringLiteral("title")].toString();
  it.link = o[QStringLiteral("link")].toString();
  it.guid = o[QStringLiteral("guid")].toString();
  it.summary = o[QStringLiteral("summary")].toString();
  it.published_at =
      QDateTime::fromString(o[QStringLiteral("published_at")].toString(), Qt::ISODate);
  it.magnet = o[QStringLiteral("magnet")].toString();
  it.torrent_url = o[QStringLiteral("torrent_url")].toString();
  it.read = o[QStringLiteral("read")].toBool(false);
  it.ignored = o[QStringLiteral("ignored")].toBool(false);
  it.downloaded = o[QStringLiteral("downloaded")].toBool(false);
  it.download_save_path = o[QStringLiteral("download_save_path")].toString();
  return it;
}

// ─── Rule serialization ───

QJsonObject ruleToJson(const RssRule& r) {
  QJsonObject o;
  o[QStringLiteral("id")] = r.id;
  o[QStringLiteral("name")] = r.name;
  o[QStringLiteral("feed_ids")] = QJsonArray::fromStringList(r.feed_ids);
  o[QStringLiteral("include_keywords")] = QJsonArray::fromStringList(r.include_keywords);
  o[QStringLiteral("exclude_keywords")] = QJsonArray::fromStringList(r.exclude_keywords);
  o[QStringLiteral("use_regex")] = r.use_regex;
  o[QStringLiteral("enabled")] = r.enabled;
  o[QStringLiteral("save_path")] = r.save_path;
  o[QStringLiteral("category")] = r.category;
  o[QStringLiteral("tags_csv")] = r.tags_csv;
  return o;
}

RssRule ruleFromJson(const QJsonObject& o) {
  RssRule r;
  r.id = o[QStringLiteral("id")].toString();
  r.name = o[QStringLiteral("name")].toString();
  for (const auto& v : o[QStringLiteral("feed_ids")].toArray())
    r.feed_ids.push_back(v.toString());
  for (const auto& v : o[QStringLiteral("include_keywords")].toArray())
    r.include_keywords.push_back(v.toString());
  for (const auto& v : o[QStringLiteral("exclude_keywords")].toArray())
    r.exclude_keywords.push_back(v.toString());
  r.use_regex = o[QStringLiteral("use_regex")].toBool(false);
  r.enabled = o[QStringLiteral("enabled")].toBool(false);
  r.save_path = o[QStringLiteral("save_path")].toString();
  r.category = o[QStringLiteral("category")].toString();
  r.tags_csv = o[QStringLiteral("tags_csv")].toString();
  return r;
}

// ─── Series serialization ───

QJsonObject seriesToJson(const SeriesSubscription& s) {
  QJsonObject o;
  o[QStringLiteral("id")] = s.id;
  o[QStringLiteral("name")] = s.name;
  o[QStringLiteral("aliases")] = QJsonArray::fromStringList(s.aliases);
  o[QStringLiteral("quality_keywords")] = QJsonArray::fromStringList(s.quality_keywords);
  o[QStringLiteral("enabled")] = s.enabled;
  o[QStringLiteral("auto_download_enabled")] = s.auto_download_enabled;
  o[QStringLiteral("season")] = s.season;
  o[QStringLiteral("last_episode_num")] = s.last_episode_num;
  o[QStringLiteral("last_episode")] = s.last_episode;
  o[QStringLiteral("save_path")] = s.save_path;
  return o;
}

SeriesSubscription seriesFromJson(const QJsonObject& o) {
  SeriesSubscription s;
  s.id = o[QStringLiteral("id")].toString();
  s.name = o[QStringLiteral("name")].toString();
  for (const auto& v : o[QStringLiteral("aliases")].toArray())
    s.aliases.push_back(v.toString());
  for (const auto& v : o[QStringLiteral("quality_keywords")].toArray())
    s.quality_keywords.push_back(v.toString());
  s.enabled = o[QStringLiteral("enabled")].toBool(true);
  s.auto_download_enabled = o[QStringLiteral("auto_download_enabled")].toBool(false);
  s.season = o[QStringLiteral("season")].toInt(-1);
  s.last_episode_num = o[QStringLiteral("last_episode_num")].toInt(0);
  s.last_episode = o[QStringLiteral("last_episode")].toString();
  s.save_path = o[QStringLiteral("save_path")].toString();
  return s;
}

// ─── Generic envelope IO ───

template <typename T, typename ToJson>
bool saveEnvelope(const QString& path, const std::vector<T>& vec, ToJson toJson) {
  QJsonArray arr;
  for (const auto& v : vec)
    arr.push_back(toJson(v));
  const QByteArray payload = QJsonDocument(arr).toJson(QJsonDocument::Compact);
  QJsonObject envelope;
  envelope[QStringLiteral("version")] = kRssJsonVersion;
  envelope[QStringLiteral("checksum")] = pfd::base::sha256Hex(payload);
  envelope[QStringLiteral("data")] = arr;
  return pfd::base::writeWholeFileWithBackup(
             path, QJsonDocument(envelope).toJson(QJsonDocument::Indented))
      .isOk();
}

template <typename T, typename FromJson>
std::vector<T> loadEnvelope(const QString& path, FromJson fromJson) {
  const auto result = pfd::base::readWholeFileWithRecovery(path);
  if (result.error.hasError() || result.data.isEmpty())
    return {};

  const auto doc = QJsonDocument::fromJson(result.data);
  QJsonArray arr;

  if (doc.isObject()) {
    const auto env = doc.object();
    arr = env[QStringLiteral("data")].toArray();
    const QString stored = env[QStringLiteral("checksum")].toString().trimmed();
    if (!stored.isEmpty()) {
      const QByteArray payload = QJsonDocument(arr).toJson(QJsonDocument::Compact);
      if (pfd::base::sha256Hex(payload) != stored) {
        LOG_WARN(QStringLiteral("[rss] Checksum mismatch in %1, loading data anyway").arg(path));
      }
    }
  } else if (doc.isArray()) {
    arr = doc.array();
  }

  std::vector<T> out;
  out.reserve(static_cast<std::size_t>(arr.size()));
  for (const auto& v : arr) {
    if (v.isObject())
      out.push_back(fromJson(v.toObject()));
  }
  return out;
}

}  // namespace

RssRepository::RssRepository(QString data_dir) : data_dir_(std::move(data_dir)) {
  QDir().mkpath(data_dir_);
}

std::vector<RssFeed> RssRepository::loadFeeds() const {
  return loadEnvelope<RssFeed>(feedsPath(), feedFromJson);
}
std::vector<RssItem> RssRepository::loadItems() const {
  return loadEnvelope<RssItem>(itemsPath(), itemFromJson);
}
std::vector<RssRule> RssRepository::loadRules() const {
  return loadEnvelope<RssRule>(rulesPath(), ruleFromJson);
}
std::vector<SeriesSubscription> RssRepository::loadSeries() const {
  return loadEnvelope<SeriesSubscription>(seriesPath(), seriesFromJson);
}

bool RssRepository::saveFeeds(const std::vector<RssFeed>& feeds) const {
  return saveEnvelope(feedsPath(), feeds, feedToJson);
}
bool RssRepository::saveItems(const std::vector<RssItem>& items) const {
  return saveEnvelope(itemsPath(), items, itemToJson);
}
bool RssRepository::saveRules(const std::vector<RssRule>& rules) const {
  return saveEnvelope(rulesPath(), rules, ruleToJson);
}
bool RssRepository::saveSeries(const std::vector<SeriesSubscription>& series) const {
  return saveEnvelope(seriesPath(), series, seriesToJson);
}

RssSettings RssRepository::loadSettings() const {
  const auto result = pfd::base::readWholeFileWithRecovery(settingsPath());
  if (result.error.hasError() || result.data.isEmpty())
    return {};
  const auto doc = QJsonDocument::fromJson(result.data);
  if (!doc.isObject())
    return {};
  const auto o = doc.object();
  RssSettings s;
  s.global_auto_download = o[QStringLiteral("global_auto_download")].toBool(false);
  s.refresh_interval_minutes = o[QStringLiteral("refresh_interval_minutes")].toInt(30);
  s.max_auto_downloads_per_refresh =
      o[QStringLiteral("max_auto_downloads_per_refresh")].toInt(kAutoDownloadMaxPerRefresh);
  s.history_max_items = o[QStringLiteral("history_max_items")].toInt(5000);
  s.history_max_age_days = o[QStringLiteral("history_max_age_days")].toInt(90);
  s.refresh_interval_minutes = std::clamp(s.refresh_interval_minutes, 5, 1440);
  s.max_auto_downloads_per_refresh = std::clamp(s.max_auto_downloads_per_refresh, 1, 100);
  s.history_max_items = std::clamp(s.history_max_items, 100, 100000);
  s.history_max_age_days = std::clamp(s.history_max_age_days, 0, 3650);
  s.external_player_command = o[QStringLiteral("external_player_command")].toString().trimmed();
  return s;
}

bool RssRepository::saveSettings(const RssSettings& s) const {
  QJsonObject o;
  o[QStringLiteral("global_auto_download")] = s.global_auto_download;
  o[QStringLiteral("refresh_interval_minutes")] = s.refresh_interval_minutes;
  o[QStringLiteral("max_auto_downloads_per_refresh")] = s.max_auto_downloads_per_refresh;
  o[QStringLiteral("history_max_items")] = s.history_max_items;
  o[QStringLiteral("history_max_age_days")] = s.history_max_age_days;
  o[QStringLiteral("external_player_command")] = s.external_player_command.trimmed();
  return pfd::base::writeWholeFileWithBackup(settingsPath(),
                                             QJsonDocument(o).toJson(QJsonDocument::Indented))
      .isOk();
}

QString RssRepository::feedsPath() const {
  return QDir(data_dir_).filePath(QStringLiteral("rss_feeds.json"));
}
QString RssRepository::itemsPath() const {
  return QDir(data_dir_).filePath(QStringLiteral("rss_items.json"));
}
QString RssRepository::rulesPath() const {
  return QDir(data_dir_).filePath(QStringLiteral("rss_rules.json"));
}
QString RssRepository::seriesPath() const {
  return QDir(data_dir_).filePath(QStringLiteral("rss_series.json"));
}
QString RssRepository::settingsPath() const {
  return QDir(data_dir_).filePath(QStringLiteral("rss_settings.json"));
}

}  // namespace pfd::core::rss
