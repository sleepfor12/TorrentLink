#include <QtCore/QDateTime>

#include <gtest/gtest.h>

#include "core/rss/rss_dedup.h"

namespace {

using pfd::core::rss::HistoryPolicy;
using pfd::core::rss::RssDedup;
using pfd::core::rss::RssItem;

RssItem makeItem(const QString& id, const QString& guid = {}, const QString& link = {},
                 const QString& magnet = {}) {
  RssItem it;
  it.id = id;
  it.guid = guid;
  it.link = link;
  it.magnet = magnet;
  it.published_at = QDateTime::currentDateTime();
  return it;
}

TEST(RssDedup, DedupById) {
  RssDedup dedup;
  std::vector<RssItem> existing = {makeItem(QStringLiteral("a"))};
  dedup.buildIndex(existing);

  EXPECT_TRUE(dedup.isDuplicate(makeItem(QStringLiteral("a"))));
  EXPECT_FALSE(dedup.isDuplicate(makeItem(QStringLiteral("b"))));
}

TEST(RssDedup, DedupByGuid) {
  RssDedup dedup;
  std::vector<RssItem> existing = {makeItem(QStringLiteral("a"), QStringLiteral("guid-1"))};
  dedup.buildIndex(existing);

  auto newItem = makeItem(QStringLiteral("different-id"), QStringLiteral("guid-1"));
  EXPECT_TRUE(dedup.isDuplicate(newItem));
}

TEST(RssDedup, DedupByLink) {
  RssDedup dedup;
  auto existing = makeItem(QStringLiteral("a"), {}, QStringLiteral("https://example.com/item1"));
  dedup.buildIndex({existing});

  auto newItem = makeItem(QStringLiteral("b"), {}, QStringLiteral("https://example.com/item1"));
  EXPECT_TRUE(dedup.isDuplicate(newItem));
}

TEST(RssDedup, DedupByInfoHash) {
  RssDedup dedup;
  const QString magnet =
      QStringLiteral("magnet:?xt=urn:btih:da39a3ee5e6b4b0d3255bfef95601890afd80709&dn=test");
  auto existing = makeItem(QStringLiteral("a"), {}, {}, magnet);
  dedup.buildIndex({existing});

  auto newItem = makeItem(
      QStringLiteral("new-id"), {}, {},
      QStringLiteral("magnet:?xt=urn:btih:DA39A3EE5E6B4B0D3255BFEF95601890AFD80709&dn=other"));
  EXPECT_TRUE(dedup.isDuplicate(newItem));
}

TEST(RssDedup, ExtractInfoHash) {
  EXPECT_EQ(RssDedup::extractInfoHash(QStringLiteral(
                "magnet:?xt=urn:btih:da39a3ee5e6b4b0d3255bfef95601890afd80709&dn=test")),
            QStringLiteral("da39a3ee5e6b4b0d3255bfef95601890afd80709"));
  EXPECT_TRUE(RssDedup::extractInfoHash(QStringLiteral("magnet:?dn=test")).isEmpty());
}

TEST(RssDedup, FilterDuplicates) {
  std::vector<RssItem> existing = {makeItem(QStringLiteral("a"))};
  std::vector<RssItem> incoming = {makeItem(QStringLiteral("a")), makeItem(QStringLiteral("b")),
                                   makeItem(QStringLiteral("c"))};
  auto unique = RssDedup::filterDuplicates(existing, incoming);
  EXPECT_EQ(unique.size(), 2u);
  EXPECT_TRUE(incoming.empty());
}

TEST(RssDedup, FilterDuplicatesInternal) {
  std::vector<RssItem> existing;
  std::vector<RssItem> incoming = {makeItem(QStringLiteral("a")), makeItem(QStringLiteral("a")),
                                   makeItem(QStringLiteral("b"))};
  auto unique = RssDedup::filterDuplicates(existing, incoming);
  EXPECT_EQ(unique.size(), 2u);
}

TEST(RssDedup, PruneByAge) {
  std::vector<RssItem> items;
  auto old_item = makeItem(QStringLiteral("old"));
  old_item.published_at = QDateTime::currentDateTime().addDays(-100);
  items.push_back(old_item);

  auto new_item = makeItem(QStringLiteral("new"));
  new_item.published_at = QDateTime::currentDateTime().addDays(-1);
  items.push_back(new_item);

  HistoryPolicy policy;
  policy.max_age_days = 90;
  policy.max_items = 10000;
  int removed = RssDedup::pruneHistory(items, policy);
  EXPECT_EQ(removed, 1);
  EXPECT_EQ(items.size(), 1u);
  EXPECT_EQ(items[0].id, QStringLiteral("new"));
}

TEST(RssDedup, PrunePreservesDownloaded) {
  std::vector<RssItem> items;
  auto old_downloaded = makeItem(QStringLiteral("old-dl"));
  old_downloaded.published_at = QDateTime::currentDateTime().addDays(-100);
  old_downloaded.downloaded = true;
  items.push_back(old_downloaded);

  auto old_not_dl = makeItem(QStringLiteral("old-no-dl"));
  old_not_dl.published_at = QDateTime::currentDateTime().addDays(-100);
  items.push_back(old_not_dl);

  HistoryPolicy policy;
  policy.max_age_days = 90;
  policy.max_items = 10000;
  int removed = RssDedup::pruneHistory(items, policy);
  EXPECT_EQ(removed, 1);
  EXPECT_EQ(items.size(), 1u);
  EXPECT_EQ(items[0].id, QStringLiteral("old-dl"));
}

TEST(RssDedup, PruneByMaxItems) {
  std::vector<RssItem> items;
  for (int i = 0; i < 10; ++i) {
    auto it = makeItem(QStringLiteral("item-%1").arg(i));
    it.published_at = QDateTime::currentDateTime().addSecs(-i * 60);
    items.push_back(it);
  }
  HistoryPolicy policy;
  policy.max_age_days = 0;
  policy.max_items = 5;
  int removed = RssDedup::pruneHistory(items, policy);
  EXPECT_EQ(removed, 5);
  EXPECT_EQ(items.size(), 5u);
}

}  // namespace
