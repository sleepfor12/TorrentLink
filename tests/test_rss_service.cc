#include <QtCore/QDir>
#include <QtCore/QUuid>

#include <gtest/gtest.h>

#include "core/rss/rss_repository.h"
#include "core/rss/rss_service.h"

namespace {

QString makeTempServiceDir() {
  const QString path = QDir::tempPath() + QStringLiteral("/pfd_rss_service_test_") +
                       QUuid::createUuid().toString(QUuid::WithoutBraces);
  QDir().mkpath(path);
  return path;
}

TEST(RssService, UpsertAndEvaluateRule) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssService svc(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("feed-1");
  feed.title = QStringLiteral("Feed 1");
  feed.url = QStringLiteral("https://example.com/rss.xml");
  feed.auto_download_enabled = true;
  svc.upsertFeed(feed);

  pfd::core::rss::RssRule rule;
  rule.id = QStringLiteral("rule-1");
  rule.name = QStringLiteral("1080p Rule");
  rule.enabled = true;
  rule.feed_ids = {feed.id};
  rule.include_keywords = {QStringLiteral("1080p")};
  rule.exclude_keywords = {QStringLiteral("x265")};
  svc.upsertRule(rule);

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("item-1");
  item.feed_id = feed.id;
  item.title = QStringLiteral("My Show S01E01 1080p x264");
  item.magnet = QStringLiteral("magnet:?xt=urn:btih:abc");

  const auto matches = svc.evaluateItem(item);
  ASSERT_EQ(matches.size(), 1u);
  EXPECT_TRUE(matches[0].matched);

  svc.setAutoDownloadEnabled(true);
  EXPECT_TRUE(svc.autoDownloadEnabled());
  EXPECT_TRUE(svc.settings().global_auto_download);
}

TEST(RssService, ApplySettingsNormalizesValues) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssService svc(dir);

  pfd::core::rss::RssSettings s;
  s.global_auto_download = true;
  s.refresh_interval_minutes = 1;
  s.max_auto_downloads_per_refresh = 500;
  s.history_max_items = 1;
  s.history_max_age_days = 50000;
  s.external_player_command = QStringLiteral("  mpv --force-window=yes  ");
  svc.applySettings(s);

  const auto normalized = svc.settings();
  EXPECT_TRUE(normalized.global_auto_download);
  EXPECT_EQ(normalized.refresh_interval_minutes, 5);
  EXPECT_EQ(normalized.max_auto_downloads_per_refresh, 100);
  EXPECT_EQ(normalized.history_max_items, 100);
  EXPECT_EQ(normalized.history_max_age_days, 3650);
  EXPECT_EQ(normalized.external_player_command, QStringLiteral("mpv --force-window=yes"));
}

TEST(RssService, SetMaxAutoDownloadsPerRefreshAppliesClamp) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssService svc(dir);

  svc.setMaxAutoDownloadsPerRefresh(-5);
  EXPECT_EQ(svc.settings().max_auto_downloads_per_refresh, 1);

  svc.setMaxAutoDownloadsPerRefresh(999);
  EXPECT_EQ(svc.settings().max_auto_downloads_per_refresh, 100);
}

TEST(RssService, DownloadItemTriggersCallbackAndMarksDownloaded) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("item-1");
  item.feed_id = QStringLiteral("feed-1");
  item.title = QStringLiteral("Item");
  item.magnet = QStringLiteral("magnet:?xt=urn:btih:abc");
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();

  int calls = 0;
  QString receivedMagnet;
  pfd::core::rss::RssDownloadSettlement settle;
  svc.setDownloadRequestCallback([&](const pfd::core::rss::AutoDownloadRequest& req) {
    ++calls;
    receivedMagnet = req.magnet;
    settle = req.rss_settlement;
  });

  EXPECT_TRUE(svc.downloadItem(item.id));
  ASSERT_EQ(calls, 1);
  EXPECT_EQ(receivedMagnet, item.magnet);
  ASSERT_EQ(svc.items().size(), 1u);
  EXPECT_FALSE(svc.items()[0].downloaded);
  svc.applyRssDownloadSettlement(settle, true);
  EXPECT_TRUE(svc.items()[0].downloaded);
}

TEST(RssService, DownloadItemTorrentUrlTriggersCallbackAndMarksDownloaded) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("item-t1");
  item.feed_id = QStringLiteral("feed-1");
  item.title = QStringLiteral("Torrent only");
  item.torrent_url = QStringLiteral("https://example.com/a.torrent");
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();

  int calls = 0;
  QString receivedUrl;
  pfd::core::rss::RssDownloadSettlement settle;
  svc.setDownloadRequestCallback([&](const pfd::core::rss::AutoDownloadRequest& req) {
    ++calls;
    receivedUrl = req.torrent_url;
    settle = req.rss_settlement;
  });

  EXPECT_TRUE(svc.downloadItem(item.id));
  ASSERT_EQ(calls, 1);
  EXPECT_EQ(receivedUrl, item.torrent_url);
  EXPECT_FALSE(svc.items()[0].downloaded);
  svc.applyRssDownloadSettlement(settle, true);
  EXPECT_TRUE(svc.items()[0].downloaded);
}

TEST(RssService, DownloadItemFillsRefererAndRuleSavePath) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("feed-1");
  feed.url = QStringLiteral("https://example.com/my.xml");
  ASSERT_TRUE(repo.saveFeeds({feed}));

  pfd::core::rss::RssRule rule;
  rule.id = QStringLiteral("rule-1");
  rule.name = QStringLiteral("All");
  rule.enabled = true;
  rule.save_path = QStringLiteral("/data/anime");
  rule.category = QStringLiteral("rss-cat");
  rule.tags_csv = QStringLiteral("a,b");
  ASSERT_TRUE(repo.saveRules({rule}));

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("item-1");
  item.feed_id = feed.id;
  item.title = QStringLiteral("Hello");
  item.magnet = QStringLiteral("magnet:?xt=urn:btih:abc");
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();

  pfd::core::rss::AutoDownloadRequest captured;
  svc.setDownloadRequestCallback(
      [&](const pfd::core::rss::AutoDownloadRequest& req) { captured = req; });

  EXPECT_TRUE(svc.downloadItem(item.id));
  EXPECT_EQ(captured.referer_url, feed.url);
  EXPECT_EQ(captured.save_path, rule.save_path);
  EXPECT_EQ(captured.rss_settlement.record_save_path, rule.save_path);
  EXPECT_EQ(captured.category, rule.category);
  EXPECT_EQ(captured.tags_csv, rule.tags_csv);
  EXPECT_TRUE(captured.add_without_interactive_confirm);
}

TEST(RssService, ApplyRssDownloadSettlementAdvancesSeriesOnSuccess) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("i1");
  item.feed_id = QStringLiteral("f");
  item.title = QStringLiteral("Ep");
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::SeriesSubscription sub;
  sub.id = QStringLiteral("s1");
  sub.name = QStringLiteral("Show");
  sub.last_episode_num = 1;
  sub.last_episode = QStringLiteral("S01E01");
  ASSERT_TRUE(repo.saveSeries({sub}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();

  pfd::core::rss::RssDownloadSettlement s;
  s.item_id = item.id;
  s.record_save_path = QStringLiteral("/tmp");
  s.series_sub_id = sub.id;
  s.series_episode = 5;
  s.series_season = 1;

  svc.applyRssDownloadSettlement(s, false);
  EXPECT_EQ(svc.series()[0].last_episode_num, 1);

  svc.applyRssDownloadSettlement(s, true);
  EXPECT_TRUE(svc.items()[0].downloaded);
  EXPECT_EQ(svc.series()[0].last_episode_num, 5);
  EXPECT_EQ(svc.series()[0].last_episode, QStringLiteral("S01E05"));
}

TEST(RssService, ApplyRssDownloadSettlementUsesOverridePath) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("p1");
  item.feed_id = QStringLiteral("f");
  item.title = QStringLiteral("T");
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();

  pfd::core::rss::RssDownloadSettlement s;
  s.item_id = item.id;
  s.record_save_path = QStringLiteral("/from_rule");

  svc.applyRssDownloadSettlement(s, true, QStringLiteral("/user_chose"));
  EXPECT_EQ(svc.items()[0].download_save_path, QStringLiteral("/user_chose"));
}

TEST(RssService, ClearItemsForFeedRemovesOnlyThatFeed) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed fa;
  fa.id = QStringLiteral("feed-a");
  fa.url = QStringLiteral("https://a/rss");
  pfd::core::rss::RssFeed fb;
  fb.id = QStringLiteral("feed-b");
  fb.url = QStringLiteral("https://b/rss");
  ASSERT_TRUE(repo.saveFeeds({fa, fb}));

  pfd::core::rss::RssItem ia;
  ia.id = QStringLiteral("item-a");
  ia.feed_id = fa.id;
  ia.title = QStringLiteral("A");
  pfd::core::rss::RssItem ib;
  ib.id = QStringLiteral("item-b");
  ib.feed_id = fb.id;
  ib.title = QStringLiteral("B");
  ASSERT_TRUE(repo.saveItems({ia, ib}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  ASSERT_EQ(svc.items().size(), 2u);

  svc.clearItemsForFeed(fa.id);
  ASSERT_EQ(svc.items().size(), 1u);
  EXPECT_EQ(svc.items()[0].feed_id, fb.id);

  svc.clearItemsForFeeds({fb.id});
  EXPECT_TRUE(svc.items().empty());
}

TEST(RssService, UpsertFeedDeduplicatesByUrl) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssService svc(dir);

  pfd::core::rss::RssFeed a;
  a.url = QStringLiteral("https://example.com/feed.xml");
  a.title = QStringLiteral("First");
  svc.upsertFeed(a);
  ASSERT_EQ(svc.feeds().size(), 1u);
  EXPECT_EQ(svc.feeds()[0].title, QStringLiteral("First"));

  pfd::core::rss::RssFeed b;
  b.url = QStringLiteral("https://example.com/feed.xml");
  b.title = QStringLiteral("Second");
  svc.upsertFeed(b);
  ASSERT_EQ(svc.feeds().size(), 1u);
  EXPECT_EQ(svc.feeds()[0].title, QStringLiteral("Second"));
}

}  // namespace
