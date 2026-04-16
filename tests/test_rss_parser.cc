#include <gtest/gtest.h>

#include "core/rss/rss_parser.h"

namespace {

TEST(RssParser, ParseRss2WithMagnetAndTorrent) {
  const QByteArray xml = R"(
<rss version="2.0">
  <channel>
    <title>Demo</title>
    <item>
      <title>Episode 01</title>
      <link>https://example.com/post/1</link>
      <guid>guid-1</guid>
      <description>desc magnet:?xt=urn:btih:abc123</description>
      <pubDate>Tue, 01 Jan 2024 12:00:00 GMT</pubDate>
      <enclosure url="https://example.com/file.torrent" />
    </item>
  </channel>
</rss>)";

  pfd::core::rss::RssParser parser;
  const auto result = parser.parse(QStringLiteral("feed-a"), xml);

  ASSERT_TRUE(result.ok);
  ASSERT_EQ(result.items.size(), 1u);
  EXPECT_EQ(result.items[0].feed_id, QStringLiteral("feed-a"));
  EXPECT_EQ(result.items[0].title, QStringLiteral("Episode 01"));
  EXPECT_EQ(result.items[0].guid, QStringLiteral("guid-1"));
  EXPECT_EQ(result.items[0].torrent_url, QStringLiteral("https://example.com/file.torrent"));
  EXPECT_TRUE(result.items[0].magnet.startsWith(QStringLiteral("magnet:?xt=urn:btih:")));
}

TEST(RssParser, ParseRss2LinkOnlyTorrentUrlMikanStyle) {
  const QByteArray xml = R"(
<rss version="2.0">
  <channel>
    <title>Mikan</title>
    <item>
      <title>Episode 99</title>
      <link>https://mikanani.me/Download/20260322/af9abf36385f132fd61ce0c49d998945b9b63aa8.torrent</link>
      <guid>ep-99</guid>
      <pubDate>Tue, 01 Jan 2024 12:00:00 GMT</pubDate>
    </item>
  </channel>
</rss>)";

  pfd::core::rss::RssParser parser;
  const auto result = parser.parse(QStringLiteral("feed-mikan"), xml);

  ASSERT_TRUE(result.ok);
  ASSERT_EQ(result.items.size(), 1u);
  EXPECT_EQ(result.items[0].torrent_url,
            QStringLiteral("https://mikanani.me/Download/20260322/"
                           "af9abf36385f132fd61ce0c49d998945b9b63aa8.torrent"));
  EXPECT_EQ(result.items[0].link, result.items[0].torrent_url);
  EXPECT_TRUE(result.items[0].magnet.isEmpty());
}

TEST(RssParser, ParseRss2EnclosureTorrentByMimeWithoutSuffix) {
  const QByteArray xml = R"(
<rss version="2.0">
  <channel>
    <title>T</title>
    <item>
      <title>BT item</title>
      <link>https://example.com/page</link>
      <guid>g-bt</guid>
      <enclosure url="https://cdn.example.com/dl/abc123" type="application/x-bittorrent" />
    </item>
  </channel>
</rss>)";

  pfd::core::rss::RssParser parser;
  const auto result = parser.parse(QStringLiteral("feed-bt"), xml);

  ASSERT_TRUE(result.ok);
  ASSERT_EQ(result.items.size(), 1u);
  EXPECT_EQ(result.items[0].torrent_url, QStringLiteral("https://cdn.example.com/dl/abc123"));
  EXPECT_EQ(result.items[0].link, QStringLiteral("https://example.com/page"));
}

TEST(RssParser, ParseRss2TorrentUrlOnlyInDescriptionHtml) {
  const QByteArray xml = R"(
<rss version="2.0">
  <channel>
    <title>HTML desc</title>
    <item>
      <title>Show 01</title>
      <link>https://example.com/ep1</link>
      <guid>g-html</guid>
      <description><![CDATA[<a href="https://mikanani.me/Download/20260322/af9abf36385f132fd61ce0c49d998945b9b63aa8.torrent">torrent</a>]]></description>
    </item>
  </channel>
</rss>)";

  pfd::core::rss::RssParser parser;
  const auto result = parser.parse(QStringLiteral("feed-html"), xml);

  ASSERT_TRUE(result.ok);
  ASSERT_EQ(result.items.size(), 1u);
  EXPECT_EQ(result.items[0].link, QStringLiteral("https://example.com/ep1"));
  EXPECT_TRUE(result.items[0].magnet.isEmpty());
  EXPECT_EQ(result.items[0].torrent_url,
            QStringLiteral("https://mikanani.me/Download/20260322/"
                           "af9abf36385f132fd61ce0c49d998945b9b63aa8.torrent"));
}

TEST(RssParser, ParseAtomBasicEntry) {
  const QByteArray xml = R"(
<feed xmlns="http://www.w3.org/2005/Atom">
  <title>Atom Feed</title>
  <entry>
    <title>Atom Item</title>
    <id>atom-guid-1</id>
    <link href="magnet:?xt=urn:btih:atom123" />
    <updated>2024-02-02T10:11:12Z</updated>
    <summary>Hello Atom</summary>
  </entry>
</feed>)";

  pfd::core::rss::RssParser parser;
  const auto result = parser.parse(QStringLiteral("feed-b"), xml);

  ASSERT_TRUE(result.ok);
  ASSERT_EQ(result.items.size(), 1u);
  EXPECT_EQ(result.items[0].guid, QStringLiteral("atom-guid-1"));
  EXPECT_EQ(result.items[0].title, QStringLiteral("Atom Item"));
  EXPECT_EQ(result.items[0].magnet, QStringLiteral("magnet:?xt=urn:btih:atom123"));
}

}  // namespace
