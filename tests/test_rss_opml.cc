#include <gtest/gtest.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QUuid>

#include "core/rss/rss_opml.h"

namespace {

using pfd::core::rss::RssFeed;
using pfd::core::rss::RssOpml;

const QString kTestOpml = QStringLiteral(
    R"(<?xml version="1.0" encoding="UTF-8"?>
<opml version="2.0">
  <head><title>Test</title></head>
  <body>
    <outline type="rss" text="Feed A" title="Feed A" xmlUrl="https://a.com/rss.xml"/>
    <outline type="rss" text="Feed B" xmlUrl="https://b.com/feed"/>
    <outline text="Folder">
      <outline type="rss" title="Nested" xmlUrl="https://c.com/rss"/>
    </outline>
    <outline text="NoUrl"/>
  </body>
</opml>)");

TEST(RssOpml, ImportFromString) {
  auto result = RssOpml::importFromString(kTestOpml);
  EXPECT_TRUE(result.ok());
  EXPECT_EQ(result.feeds.size(), 3u);
  EXPECT_EQ(result.skipped, 2);

  EXPECT_EQ(result.feeds[0].url, QStringLiteral("https://a.com/rss.xml"));
  EXPECT_EQ(result.feeds[0].title, QStringLiteral("Feed A"));
  EXPECT_EQ(result.feeds[1].url, QStringLiteral("https://b.com/feed"));
  EXPECT_EQ(result.feeds[1].title, QStringLiteral("Feed B"));
  EXPECT_EQ(result.feeds[2].url, QStringLiteral("https://c.com/rss"));
  EXPECT_EQ(result.feeds[2].title, QStringLiteral("Nested"));

  for (const auto& f : result.feeds) {
    EXPECT_FALSE(f.id.isEmpty());
  }
}

TEST(RssOpml, ExportToString) {
  std::vector<RssFeed> feeds;
  {
    RssFeed f;
    f.id = QStringLiteral("1");
    f.title = QStringLiteral("Test Feed");
    f.url = QStringLiteral("https://example.com/rss");
    feeds.push_back(f);
  }
  {
    RssFeed f;
    f.id = QStringLiteral("2");
    f.title = QStringLiteral("Another Feed");
    f.url = QStringLiteral("https://other.com/feed.xml");
    feeds.push_back(f);
  }

  const QString xml = RssOpml::exportToString(feeds);
  EXPECT_TRUE(xml.contains(QStringLiteral("xmlUrl=\"https://example.com/rss\"")));
  EXPECT_TRUE(xml.contains(QStringLiteral("xmlUrl=\"https://other.com/feed.xml\"")));
  EXPECT_TRUE(xml.contains(QStringLiteral("title=\"Test Feed\"")));
  EXPECT_TRUE(xml.contains(QStringLiteral("<opml")));
}

TEST(RssOpml, RoundTrip) {
  std::vector<RssFeed> original;
  for (int i = 0; i < 5; ++i) {
    RssFeed f;
    f.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    f.title = QStringLiteral("Feed %1").arg(i);
    f.url = QStringLiteral("https://example.com/feed%1").arg(i);
    original.push_back(f);
  }

  const QString xml = RssOpml::exportToString(original);
  auto imported = RssOpml::importFromString(xml);
  EXPECT_TRUE(imported.ok());
  ASSERT_EQ(imported.feeds.size(), original.size());
  for (std::size_t i = 0; i < original.size(); ++i) {
    EXPECT_EQ(imported.feeds[i].url, original[i].url);
    EXPECT_EQ(imported.feeds[i].title, original[i].title);
  }
}

TEST(RssOpml, ImportFileNotFound) {
  auto result = RssOpml::importFromFile(QStringLiteral("/nonexistent/path.opml"));
  EXPECT_FALSE(result.ok());
}

TEST(RssOpml, ExportAndImportFile) {
  const QString dir = QDir::tempPath() + QStringLiteral("/pfd_opml_test_") +
                      QUuid::createUuid().toString(QUuid::WithoutBraces);
  QDir().mkpath(dir);
  const QString path = dir + QStringLiteral("/test.opml");

  std::vector<RssFeed> feeds;
  RssFeed f;
  f.id = QStringLiteral("1");
  f.title = QStringLiteral("File Test");
  f.url = QStringLiteral("https://test.com/rss");
  feeds.push_back(f);

  EXPECT_TRUE(RssOpml::exportToFile(path, feeds));

  auto result = RssOpml::importFromFile(path);
  EXPECT_TRUE(result.ok());
  ASSERT_EQ(result.feeds.size(), 1u);
  EXPECT_EQ(result.feeds[0].url, QStringLiteral("https://test.com/rss"));

  QFile::remove(path);
  QDir().rmdir(dir);
}

}  // namespace
