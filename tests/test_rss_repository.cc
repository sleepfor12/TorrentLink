#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUuid>

#include <gtest/gtest.h>

#include "base/io.h"
#include "core/rss/rss_repository.h"

namespace {

QString makeTempRepoDir() {
  const QString path = QDir::tempPath() + QStringLiteral("/pfd_rss_repo_test_") +
                       QUuid::createUuid().toString(QUuid::WithoutBraces);
  QDir().mkpath(path);
  return path;
}

TEST(RssRepository, SaveAndLoadRoundTrip) {
  const QString dir = makeTempRepoDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("f1");
  feed.title = QStringLiteral("Feed 1");
  feed.url = QStringLiteral("https://example.com/rss");

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("i1");
  item.feed_id = feed.id;
  item.title = QStringLiteral("Item 1");
  item.magnet = QStringLiteral("magnet:?xt=urn:btih:123");

  pfd::core::rss::RssRule rule;
  rule.id = QStringLiteral("r1");
  rule.name = QStringLiteral("Rule 1");
  rule.enabled = true;
  rule.feed_ids = {feed.id};
  rule.include_keywords = {QStringLiteral("Item")};

  pfd::core::rss::SeriesSubscription series;
  series.id = QStringLiteral("s1");
  series.name = QStringLiteral("Series 1");
  series.auto_download_enabled = true;

  ASSERT_TRUE(repo.saveFeeds({feed}));
  ASSERT_TRUE(repo.saveItems({item}));
  ASSERT_TRUE(repo.saveRules({rule}));
  ASSERT_TRUE(repo.saveSeries({series}));

  const auto feeds = repo.loadFeeds();
  const auto items = repo.loadItems();
  const auto rules = repo.loadRules();
  const auto allSeries = repo.loadSeries();

  ASSERT_EQ(feeds.size(), 1u);
  ASSERT_EQ(items.size(), 1u);
  ASSERT_EQ(rules.size(), 1u);
  ASSERT_EQ(allSeries.size(), 1u);
  EXPECT_EQ(feeds[0].url, feed.url);
  EXPECT_EQ(items[0].magnet, item.magnet);
  EXPECT_TRUE(rules[0].enabled);
  EXPECT_TRUE(allSeries[0].auto_download_enabled);
}

TEST(RssRepository, ChecksumMismatchStillLoadsData) {
  const QString dir = makeTempRepoDir();
  pfd::core::rss::RssRepository repo(dir);
  const QString feedsPath = QDir(dir).filePath(QStringLiteral("rss_feeds.json"));

  QJsonObject feedObj;
  feedObj[QStringLiteral("id")] = QStringLiteral("f1");
  feedObj[QStringLiteral("title")] = QStringLiteral("Corrupted Feed");
  feedObj[QStringLiteral("url")] = QStringLiteral("https://example.com/rss");

  QJsonArray arr;
  arr.append(feedObj);

  QJsonObject envelope;
  envelope[QStringLiteral("version")] = 1;
  envelope[QStringLiteral("checksum")] = QStringLiteral("deadbeef");
  envelope[QStringLiteral("data")] = arr;
  ASSERT_TRUE(pfd::base::writeWholeFile(feedsPath, QJsonDocument(envelope).toJson()).isOk());

  const auto feeds = repo.loadFeeds();
  ASSERT_EQ(feeds.size(), 1u);
  EXPECT_EQ(feeds[0].id, QStringLiteral("f1"));
  EXPECT_EQ(feeds[0].title, QStringLiteral("Corrupted Feed"));
}

TEST(RssRepository, LoadSettingsAppliesClampAndTrim) {
  const QString dir = makeTempRepoDir();
  pfd::core::rss::RssRepository repo(dir);
  const QString settingsPath = QDir(dir).filePath(QStringLiteral("rss_settings.json"));

  QJsonObject o;
  o[QStringLiteral("global_auto_download")] = true;
  o[QStringLiteral("refresh_interval_minutes")] = 1;
  o[QStringLiteral("max_auto_downloads_per_refresh")] = 999;
  o[QStringLiteral("history_max_items")] = 42;
  o[QStringLiteral("history_max_age_days")] = 99999;
  o[QStringLiteral("external_player_command")] = QStringLiteral("  mpv --force-window=yes  ");
  ASSERT_TRUE(pfd::base::writeWholeFile(settingsPath, QJsonDocument(o).toJson()).isOk());

  const auto settings = repo.loadSettings();
  EXPECT_TRUE(settings.global_auto_download);
  EXPECT_EQ(settings.refresh_interval_minutes, 5);
  EXPECT_EQ(settings.max_auto_downloads_per_refresh, 100);
  EXPECT_EQ(settings.history_max_items, 100);
  EXPECT_EQ(settings.history_max_age_days, 3650);
  EXPECT_EQ(settings.external_player_command, QStringLiteral("mpv --force-window=yes"));
}

TEST(RssRepository, SaveSettingsTrimsExternalPlayerCommand) {
  const QString dir = makeTempRepoDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssSettings s;
  s.external_player_command = QStringLiteral("  vlc  ");
  ASSERT_TRUE(repo.saveSettings(s));

  const auto loaded = repo.loadSettings();
  EXPECT_EQ(loaded.external_player_command, QStringLiteral("vlc"));
}

}  // namespace
