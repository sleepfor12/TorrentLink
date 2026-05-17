#include <QtCore/QDir>
#include <QtCore/QUuid>

#include <gtest/gtest.h>

#include "core/rss/rss_repository.h"
#include "core/rss/rss_service.h"

namespace {

QString makeTempDir() {
  const QString path = QDir::tempPath() + QStringLiteral("/pfd_rss_ad_test_") +
                       QUuid::createUuid().toString(QUuid::WithoutBraces);
  QDir().mkpath(path);
  return path;
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
  svc.setDownloadRequestCallback(
      [&](const pfd::core::rss::AutoDownloadRequest& req) { captured = req; });

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

TEST(RssAutoDownload, RuleAutoDownloadQueuesWhenGlobalAndFeedEnabled) {
  const QString dir = makeTempDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("f1");
  feed.url = QStringLiteral("https://example.com/rss");
  feed.auto_download_enabled = true;
  ASSERT_TRUE(repo.saveFeeds({feed}));

  pfd::core::rss::RssRule rule;
  rule.id = QStringLiteral("r1");
  rule.name = QStringLiteral("CatchAll");
  rule.enabled = true;
  rule.save_path = QStringLiteral("/tmp/rss-rule-save");
  ASSERT_TRUE(repo.saveRules({rule}));

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("i1");
  item.feed_id = QStringLiteral("f1");
  item.title = QStringLiteral("Some.Release.1080p");
  item.magnet = QStringLiteral("magnet:?xt=urn:btih:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  svc.setAutoDownloadEnabled(true);

  int queued = 0;
  pfd::core::rss::AutoDownloadRequest last;
  svc.setDownloadRequestCallback([&](const pfd::core::rss::AutoDownloadRequest& req) {
    ++queued;
    last = req;
  });

  EXPECT_TRUE(svc.downloadItem(QStringLiteral("i1")));
  EXPECT_EQ(queued, 1);
  EXPECT_TRUE(last.rule_id.isEmpty());
  EXPECT_EQ(last.feed_id, QStringLiteral("f1"));
  EXPECT_EQ(last.save_path, QStringLiteral("/tmp/rss-rule-save"));
  EXPECT_TRUE(last.add_without_interactive_confirm);
}

TEST(RssAutoDownload, DiagnosticsReportNoRuleMatch) {
  const QString dir = makeTempDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("f1");
  feed.auto_download_enabled = true;
  ASSERT_TRUE(repo.saveFeeds({feed}));

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("i1");
  item.feed_id = QStringLiteral("f1");
  item.title = QStringLiteral("Unmatched title");
  item.magnet = QStringLiteral("magnet:?xt=urn:btih:cccccccccccccccccccccccccccccccccccccccc");
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  svc.setAutoDownloadEnabled(true);

  svc.refreshAutoDownloadDiagnostics();
  const auto& loaded = svc.items();
  ASSERT_EQ(loaded.size(), 1u);
  EXPECT_EQ(loaded[0].last_auto_reason_code, QStringLiteral("no_rule_match"));
}

TEST(RssAutoDownload, NoCallbackFailsDownload) {
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

  const auto ruleMatch = svc.evaluateItem(item);
  ASSERT_FALSE(ruleMatch.empty());
  EXPECT_TRUE(ruleMatch[0].matched);

  EXPECT_FALSE(svc.downloadItem(QStringLiteral("i1")));
}

}  // namespace
