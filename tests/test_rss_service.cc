#include <QtCore/QDateTime>
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
  s.max_concurrent_auto_downloads = 100;
  s.history_max_items = 1;
  s.history_max_age_days = 50000;
  s.external_player_command = QStringLiteral("  mpv --force-window=yes  ");
  svc.applySettings(s);

  const auto normalized = svc.settings();
  EXPECT_TRUE(normalized.global_auto_download);
  EXPECT_EQ(normalized.refresh_interval_minutes, 5);
  EXPECT_EQ(normalized.max_auto_downloads_per_refresh, 100);
  EXPECT_EQ(normalized.max_concurrent_auto_downloads, 50);
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

TEST(RssService, DispatchDeferredAutoDownloadsRespectsConcurrentAndWaitlist) {
  const QString dir = makeTempServiceDir();
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

  static const char* const hashes[] = {
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
      "cccccccccccccccccccccccccccccccccccccccc", "dddddddddddddddddddddddddddddddddddddddd",
      "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"};

  std::vector<pfd::core::rss::RssItem> batch;
  for (int i = 0; i < 5; ++i) {
    pfd::core::rss::RssItem item;
    item.id = QStringLiteral("i%1").arg(i);
    item.feed_id = feed.id;
    item.title = QStringLiteral("Release %1").arg(i);
    item.magnet = QStringLiteral("magnet:?xt=urn:btih:%1").arg(QLatin1String(hashes[i]));
    item.published_at = QDateTime::fromSecsSinceEpoch(1000 + i);
    batch.push_back(std::move(item));
  }
  ASSERT_TRUE(repo.saveItems(batch));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  pfd::core::rss::RssSettings st = svc.settings();
  st.global_auto_download = true;
  st.max_concurrent_auto_downloads = 2;
  st.max_auto_downloads_per_refresh = 10;
  svc.applySettings(st);

  int calls = 0;
  svc.setDownloadRequestCallback([&](const pfd::core::rss::AutoDownloadRequest&) { ++calls; });

  EXPECT_EQ(svc.dispatchDeferredAutoDownloads(10), 2);
  EXPECT_EQ(calls, 2);

  svc.refreshAutoDownloadDiagnostics();
  int inflight = 0;
  int waitlisted = 0;
  for (const auto& it : svc.items()) {
    if (it.queued) {
      ++inflight;
    }
    if (it.rss_auto_waitlisted) {
      ++waitlisted;
    }
  }
  EXPECT_EQ(inflight, 2);
  EXPECT_EQ(waitlisted, 3);

  pfd::core::rss::RssDownloadSettlement s;
  s.item_id = QStringLiteral("i4");
  s.record_save_path = QStringLiteral("/tmp");
  svc.applyRssDownloadSettlement(s, true);
  EXPECT_EQ(svc.dispatchDeferredAutoDownloads(10), 1);
  EXPECT_EQ(calls, 3);
}

TEST(RssService, AutoDownloadBacklogCapLimitsTrackingAndPump) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("f1");
  feed.auto_download_enabled = true;
  ASSERT_TRUE(repo.saveFeeds({feed}));

  pfd::core::rss::RssRule rule;
  rule.id = QStringLiteral("r1");
  rule.name = QStringLiteral("All");
  rule.enabled = true;
  ASSERT_TRUE(repo.saveRules({rule}));

  static const char* const hashes[] = {
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
      "cccccccccccccccccccccccccccccccccccccccc",
      "dddddddddddddddddddddddddddddddddddddddd",
  };

  std::vector<pfd::core::rss::RssItem> batch;
  for (int i = 0; i < 4; ++i) {
    pfd::core::rss::RssItem item;
    item.id = QStringLiteral("i%1").arg(i);
    item.feed_id = feed.id;
    item.title = QStringLiteral("Release %1").arg(i);
    item.magnet = QStringLiteral("magnet:?xt=urn:btih:%1").arg(QLatin1String(hashes[i]));
    item.published_at = QDateTime::fromSecsSinceEpoch(1000 + i);
    batch.push_back(std::move(item));
  }
  ASSERT_TRUE(repo.saveItems(batch));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  pfd::core::rss::RssSettings st = svc.settings();
  st.global_auto_download = true;
  st.max_concurrent_auto_downloads = 3;
  st.max_auto_download_backlog = 2;
  st.max_auto_downloads_per_refresh = 10;
  svc.applySettings(st);

  int calls = 0;
  svc.setDownloadRequestCallback([&](const pfd::core::rss::AutoDownloadRequest&) { ++calls; });

  svc.refreshAutoDownloadDiagnostics();
  int overflow = 0;
  for (const auto& it : svc.items()) {
    if (it.last_auto_reason_code == QStringLiteral("auto_backlog_overflow")) {
      ++overflow;
    }
  }
  EXPECT_EQ(overflow, 2);

  EXPECT_EQ(svc.dispatchDeferredAutoDownloads(10), 2);
  EXPECT_EQ(calls, 2);
}

TEST(RssService, DispatchNewItemsOnlySkipsExistingBacklog) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("f1");
  feed.auto_download_enabled = true;
  ASSERT_TRUE(repo.saveFeeds({feed}));

  pfd::core::rss::RssRule rule;
  rule.id = QStringLiteral("r1");
  rule.name = QStringLiteral("All");
  rule.enabled = true;
  ASSERT_TRUE(repo.saveRules({rule}));

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("old1");
  item.feed_id = QStringLiteral("f1");
  item.title = QStringLiteral("Old Release");
  item.magnet = QStringLiteral("magnet:?xt=urn:btih:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  svc.setAutoDownloadEnabled(true);

  int calls = 0;
  svc.setDownloadRequestCallback([&](const pfd::core::rss::AutoDownloadRequest&) { ++calls; });

  EXPECT_TRUE(svc.lastRefreshNewItemIds().isEmpty());
  EXPECT_EQ(svc.dispatchDeferredAutoDownloads(10, true,
                                              pfd::core::rss::RssAutoDownloadScope::kNewItemsOnly),
            0);
  EXPECT_EQ(calls, 0);

  EXPECT_EQ(svc.dispatchDeferredAutoDownloads(10, true,
                                              pfd::core::rss::RssAutoDownloadScope::kAllEligible),
            1);
  EXPECT_EQ(calls, 1);
}

TEST(RssService, DownloadItemTriggersCallbackAndMarksDownloaded) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("feed-1");
  feed.url = QStringLiteral("https://example.com/rss.xml");
  ASSERT_TRUE(repo.saveFeeds({feed}));

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

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("feed-1");
  feed.url = QStringLiteral("https://example.com/rss.xml");
  ASSERT_TRUE(repo.saveFeeds({feed}));

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

TEST(RssService, ApplyRssDownloadSettlementMarksItemOnSuccess) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("f");
  feed.url = QStringLiteral("https://example.com/rss.xml");
  ASSERT_TRUE(repo.saveFeeds({feed}));

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("i1");
  item.feed_id = QStringLiteral("f");
  item.title = QStringLiteral("Release");
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();

  pfd::core::rss::RssDownloadSettlement s;
  s.item_id = item.id;
  s.record_save_path = QStringLiteral("/tmp");

  svc.applyRssDownloadSettlement(s, false);
  EXPECT_FALSE(svc.items()[0].downloaded);

  svc.applyRssDownloadSettlement(s, true, QStringLiteral("/vault/resolved"));
  EXPECT_TRUE(svc.items()[0].downloaded);
  EXPECT_EQ(svc.items()[0].download_save_path, QStringLiteral("/vault/resolved"));
}

TEST(RssService, ApplyRssDownloadSettlementUsesOverridePath) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("f");
  feed.url = QStringLiteral("https://example.com/rss.xml");
  ASSERT_TRUE(repo.saveFeeds({feed}));

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

TEST(RssService, SettlementFailureIncrementsRetryAndKeepsNotDownloaded) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("feed");
  feed.url = QStringLiteral("https://example.com/rss.xml");
  ASSERT_TRUE(repo.saveFeeds({feed}));

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("f1");
  item.feed_id = QStringLiteral("feed");
  item.title = QStringLiteral("FailItem");
  item.magnet = QStringLiteral("magnet:?xt=urn:btih:abc");
  item.queued = true;
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  pfd::core::rss::RssDownloadSettlement s;
  s.item_id = item.id;
  svc.applyRssDownloadSettlement(s, false);
  ASSERT_EQ(svc.items().size(), 1u);
  EXPECT_FALSE(svc.items()[0].downloaded);
  EXPECT_FALSE(svc.items()[0].queued);
  EXPECT_EQ(svc.items()[0].retry_count, 1);
  EXPECT_EQ(svc.items()[0].last_auto_reason_code, QStringLiteral("settlement_failed"));
}

TEST(RssService, DownloadItemSkipsWhenAlreadyQueued) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("feed");
  feed.url = QStringLiteral("https://example.com/rss.xml");
  ASSERT_TRUE(repo.saveFeeds({feed}));

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("q1");
  item.feed_id = QStringLiteral("feed");
  item.title = QStringLiteral("Queued");
  item.magnet = QStringLiteral("magnet:?xt=urn:btih:abc");
  item.queued = true;
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  int calls = 0;
  svc.setDownloadRequestCallback([&](const pfd::core::rss::AutoDownloadRequest&) { ++calls; });
  // queued is transient and reset on load, so manual click should dispatch once.
  EXPECT_TRUE(svc.downloadItem(item.id));
  EXPECT_EQ(calls, 1);
}

TEST(RssService, QueuedStateIsResetAfterReload) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("feed");
  feed.url = QStringLiteral("https://example.com/rss.xml");
  ASSERT_TRUE(repo.saveFeeds({feed}));

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("q2");
  item.feed_id = QStringLiteral("feed");
  item.title = QStringLiteral("Reload");
  item.magnet = QStringLiteral("magnet:?xt=urn:btih:abc");
  item.queued = true;
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  ASSERT_EQ(svc.items().size(), 1u);
  EXPECT_FALSE(svc.items()[0].queued);

  int calls = 0;
  svc.setDownloadRequestCallback([&](const pfd::core::rss::AutoDownloadRequest&) { ++calls; });
  EXPECT_TRUE(svc.downloadItem(item.id));
  EXPECT_EQ(calls, 1);
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
  const QString stableId = svc.feeds()[0].id;
  EXPECT_FALSE(stableId.isEmpty());

  pfd::core::rss::RssFeed b;
  b.url = QStringLiteral("https://example.com/feed.xml");
  b.title = QStringLiteral("Second");
  svc.upsertFeed(b);
  ASSERT_EQ(svc.feeds().size(), 1u);
  EXPECT_EQ(svc.feeds()[0].title, QStringLiteral("Second"));
  EXPECT_EQ(svc.feeds()[0].id, stableId);
}

TEST(RssService, LoadStatePrunesItemsForMissingFeeds) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("keep-feed");
  feed.url = QStringLiteral("https://example.com/keep.xml");
  ASSERT_TRUE(repo.saveFeeds({feed}));

  pfd::core::rss::RssItem good;
  good.id = QStringLiteral("i-good");
  good.feed_id = feed.id;
  good.title = QStringLiteral("Good");

  pfd::core::rss::RssItem orphan;
  orphan.id = QStringLiteral("i-orphan");
  orphan.feed_id = QStringLiteral("deleted-feed");
  orphan.title = QStringLiteral("Orphan");

  pfd::core::rss::RssItem noFeedId;
  noFeedId.id = QStringLiteral("i-nofeed");
  noFeedId.feed_id = QStringLiteral("");
  noFeedId.title = QStringLiteral("No feed id");

  ASSERT_TRUE(repo.saveItems({good, orphan, noFeedId}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  ASSERT_EQ(svc.items().size(), 1u);
  EXPECT_EQ(svc.items()[0].id, QStringLiteral("i-good"));
}

TEST(RssService, UpsertSecondFeedDoesNotOverwriteWhenStoredFeedHasEmptyId) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed a;
  a.id = QString();
  a.url = QStringLiteral("https://example.com/a.xml");
  ASSERT_TRUE(repo.saveFeeds({a}));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  ASSERT_EQ(svc.feeds().size(), 1u);

  pfd::core::rss::RssFeed b;
  b.url = QStringLiteral("https://example.com/b.xml");
  svc.upsertFeed(b);
  ASSERT_EQ(svc.feeds().size(), 2u);
  EXPECT_EQ(svc.feeds()[0].url, QStringLiteral("https://example.com/a.xml"));
  EXPECT_EQ(svc.feeds()[1].url, QStringLiteral("https://example.com/b.xml"));
}

TEST(RssService, UpsertByUrlMergePreservesAutoDownload) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssService svc(dir);

  pfd::core::rss::RssFeed a;
  a.url = QStringLiteral("https://example.com/feed.xml");
  a.title = QStringLiteral("My feed");
  svc.upsertFeed(a);
  ASSERT_EQ(svc.feeds().size(), 1u);
  const QString stableId = svc.feeds()[0].id;

  pfd::core::rss::RssFeed withAuto = svc.feeds()[0];
  withAuto.auto_download_enabled = true;
  svc.upsertFeed(withAuto);
  ASSERT_TRUE(svc.feeds()[0].auto_download_enabled);

  pfd::core::rss::RssFeed shell;
  shell.url = QStringLiteral("https://example.com/feed.xml");
  shell.title = QStringLiteral("https://example.com/feed.xml");
  svc.upsertFeed(shell);

  ASSERT_EQ(svc.feeds().size(), 1u);
  EXPECT_EQ(svc.feeds()[0].id, stableId);
  EXPECT_TRUE(svc.feeds()[0].auto_download_enabled);
}

TEST(RssService, RefreshDiagnosticsForFeedReflectsFeedAutoOff) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed feed;
  feed.id = QStringLiteral("fid1");
  feed.url = QStringLiteral("https://example.com/a.xml");
  feed.auto_download_enabled = true;
  ASSERT_TRUE(repo.saveFeeds({feed}));

  pfd::core::rss::RssRule rule;
  rule.id = QStringLiteral("r1");
  rule.name = QStringLiteral("all");
  rule.enabled = true;
  rule.include_keywords = QStringList{QStringLiteral("Item")};
  ASSERT_TRUE(repo.saveRules({rule}));

  pfd::core::rss::RssItem item;
  item.id = QStringLiteral("i1");
  item.feed_id = feed.id;
  item.title = QStringLiteral("Test Item");
  item.magnet = QStringLiteral("magnet:?xt=urn:btih:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  ASSERT_TRUE(repo.saveItems({item}));

  pfd::core::rss::RssSettings st;
  st.global_auto_download = true;
  ASSERT_TRUE(repo.saveSettings(st));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  svc.setDownloadRequestCallback([](const pfd::core::rss::AutoDownloadRequest&) {});

  svc.refreshAutoDownloadDiagnosticsForFeed(feed.id);
  EXPECT_EQ(svc.items()[0].last_auto_reason_code, QStringLiteral("diagnostic_rule_eligible"));

  pfd::core::rss::RssFeed f2 = *svc.findFeed(feed.id);
  f2.auto_download_enabled = false;
  svc.upsertFeed(f2);
  svc.refreshAutoDownloadDiagnosticsForFeed(feed.id);
  EXPECT_EQ(svc.items()[0].last_auto_reason_code, QStringLiteral("feed_auto_off"));
}

TEST(RssService, ReloadItemStreamFromEnabledFeedsClearsEnabledItemsOnly) {
  const QString dir = makeTempServiceDir();
  pfd::core::rss::RssRepository repo(dir);

  pfd::core::rss::RssFeed fa;
  fa.id = QStringLiteral("fa");
  fa.url = QStringLiteral("ftp://invalid.test/rss.xml");
  fa.enabled = true;
  pfd::core::rss::RssFeed fb;
  fb.id = QStringLiteral("fb");
  fb.url = QStringLiteral("https://example.com/b.xml");
  fb.enabled = false;
  ASSERT_TRUE(repo.saveFeeds({fa, fb}));

  pfd::core::rss::RssItem ia;
  ia.id = QStringLiteral("ia");
  ia.feed_id = fa.id;
  ia.title = QStringLiteral("A");
  pfd::core::rss::RssItem ib;
  ib.id = QStringLiteral("ib");
  ib.feed_id = fb.id;
  ib.title = QStringLiteral("B");
  ASSERT_TRUE(repo.saveItems({ia, ib}));

  pfd::core::rss::RssSettings st;
  st.global_auto_download = false;
  ASSERT_TRUE(repo.saveSettings(st));

  pfd::core::rss::RssService svc(dir);
  svc.loadState();
  ASSERT_EQ(svc.items().size(), 2u);

  svc.reloadItemStreamFromEnabledFeeds();

  ASSERT_EQ(svc.items().size(), 1u);
  EXPECT_EQ(svc.items()[0].feed_id, QStringLiteral("fb"));
}

}  // namespace
