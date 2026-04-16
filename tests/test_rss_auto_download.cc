#include <gtest/gtest.h>

#include <QtCore/QDir>
#include <QtCore/QUuid>

#include "core/rss/rss_service.h"

namespace {

QString makeTempDir() {
  const QString path = QDir::tempPath() + QStringLiteral("/pfd_rss_ad_test_") +
                       QUuid::createUuid().toString(QUuid::WithoutBraces);
  QDir().mkpath(path);
  return path;
}

TEST(RssAutoDownload, ThrottleLimitsDownloads) {
  const QString dir = makeTempDir();
  pfd::core::rss::RssService svc(dir);
  svc.setAutoDownloadEnabled(true);
  svc.setMaxAutoDownloadsPerRefresh(2);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("f1");
  feed.url = QStringLiteral("https://example.com/rss");
  feed.auto_download_enabled = true;
  svc.upsertFeed(feed);

  pfd::core::rss::RssRule rule;
  rule.id = QStringLiteral("r1");
  rule.name = QStringLiteral("All");
  rule.enabled = true;
  svc.upsertRule(rule);

  int downloads = 0;
  svc.setDownloadRequestCallback([&](const pfd::core::rss::AutoDownloadRequest&) {
    ++downloads;
  });

  EXPECT_EQ(downloads, 0);
}

TEST(RssAutoDownload, CallbackReceivesMetadata) {
  const QString dir = makeTempDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("f1");
  feed.url = QStringLiteral("https://example.com/feed.xml");
  ASSERT_TRUE(repo.saveFeeds({feed}));

  pfd::core::rss::RssRule rule;
  rule.id = QStringLiteral("r1");
  rule.name = QStringLiteral("All");
  rule.enabled = true;
  rule.save_path = QStringLiteral("/vault/rss");
  ASSERT_TRUE(repo.saveRules({rule}));

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("i1");
  item.feed_id = QStringLiteral("f1");
  item.title = QStringLiteral("Test Item");
  item.magnet = QStringLiteral("magnet:?xt=urn:btih:abc");
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();

  pfd::core::rss::AutoDownloadRequest captured;
  svc.setDownloadRequestCallback([&](const pfd::core::rss::AutoDownloadRequest& req) {
    captured = req;
  });

  EXPECT_TRUE(svc.downloadItem(QStringLiteral("i1")));
  EXPECT_EQ(captured.magnet, QStringLiteral("magnet:?xt=urn:btih:abc"));
  EXPECT_TRUE(captured.torrent_url.isEmpty());
  EXPECT_EQ(captured.item_id, QStringLiteral("i1"));
  EXPECT_EQ(captured.feed_id, QStringLiteral("f1"));
  EXPECT_EQ(captured.item_title, QStringLiteral("Test Item"));
  EXPECT_EQ(captured.referer_url, feed.url);
  EXPECT_EQ(captured.save_path, rule.save_path);
  EXPECT_EQ(captured.rss_settlement.item_id, QStringLiteral("i1"));
  EXPECT_EQ(captured.rss_settlement.record_save_path, rule.save_path);
}

TEST(RssAutoDownload, SeriesMatchAndAutoAdvance) {
  const QString dir = makeTempDir();
  pfd::core::rss::RssService svc(dir);
  svc.setAutoDownloadEnabled(true);

  pfd::core::rss::SeriesSubscription sub;
  sub.id = QStringLiteral("s1");
  sub.name = QStringLiteral("My Show");
  sub.enabled = true;
  sub.auto_download_enabled = true;
  sub.season = 1;
  sub.last_episode_num = 4;
  svc.upsertSeries(sub);

  auto match = svc.matchSeries([] {
    pfd::core::rss::RssItem it;
    it.id = QStringLiteral("x");
    it.feed_id = QStringLiteral("f");
    it.title = QStringLiteral("My Show S01E05 1080p");
    it.magnet = QStringLiteral("magnet:?xt=urn:btih:xyz");
    return it;
  }());

  ASSERT_TRUE(match.has_value());
  EXPECT_EQ(match->id, QStringLiteral("s1"));
}

TEST(RssAutoDownload, NoCallbackDoesNotBlockSeriesPath) {
  const QString dir = makeTempDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("f1");
  feed.url = QStringLiteral("https://example.com/rss");
  feed.auto_download_enabled = true;
  ASSERT_TRUE(repo.saveFeeds({feed}));

  pfd::core::rss::RssRule rule;
  rule.id = QStringLiteral("r1");
  rule.name = QStringLiteral("All");
  rule.enabled = true;
  ASSERT_TRUE(repo.saveRules({rule}));

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("i1");
  item.feed_id = QStringLiteral("f1");
  item.title = QStringLiteral("My Show S01E05 1080p");
  item.magnet = QStringLiteral("magnet:?xt=urn:btih:abc");
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  svc.setAutoDownloadEnabled(true);

  pfd::core::rss::SeriesSubscription sub;
  sub.id = QStringLiteral("s1");
  sub.name = QStringLiteral("My Show");
  sub.enabled = true;
  sub.auto_download_enabled = true;
  sub.season = 1;
  sub.last_episode_num = 4;
  svc.upsertSeries(sub);

  const auto ruleMatch = svc.evaluateItem(item);
  ASSERT_FALSE(ruleMatch.empty());
  EXPECT_TRUE(ruleMatch[0].matched);

  const auto seriesMatch = svc.matchSeries(item);
  ASSERT_TRUE(seriesMatch.has_value());
  EXPECT_EQ(seriesMatch->id, QStringLiteral("s1"));

  EXPECT_FALSE(svc.downloadItem(QStringLiteral("i1")));
}

}  // namespace
