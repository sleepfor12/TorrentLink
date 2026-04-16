#include <gtest/gtest.h>

#include "core/rss/rss_series_tracker.h"

namespace {

using pfd::core::rss::RssSeriesTracker;
using pfd::core::rss::SeriesSubscription;

TEST(RssSeriesTracker, ParseSxxEyy) {
  auto ep = RssSeriesTracker::parseTitle(QStringLiteral("My Show S01E05 1080p x264"));
  ASSERT_TRUE(ep.has_value());
  EXPECT_EQ(ep->season, 1);
  EXPECT_EQ(ep->episode, 5);
  EXPECT_FALSE(ep->series_name.isEmpty());
  EXPECT_EQ(ep->quality, QStringLiteral("1080P"));
}

TEST(RssSeriesTracker, ParseDashNumber) {
  auto ep = RssSeriesTracker::parseTitle(QStringLiteral("[SubGroup] My Show - 12 [1080p]"));
  ASSERT_TRUE(ep.has_value());
  EXPECT_EQ(ep->episode, 12);
  EXPECT_EQ(ep->season, -1);
}

TEST(RssSeriesTracker, ParseChineseEpisode) {
  auto ep = RssSeriesTracker::parseTitle(QStringLiteral("动漫名称 第05集 1080p"));
  ASSERT_TRUE(ep.has_value());
  EXPECT_EQ(ep->episode, 5);
}

TEST(RssSeriesTracker, ParseEPFormat) {
  auto ep = RssSeriesTracker::parseTitle(QStringLiteral("Show Name EP03 720p"));
  ASSERT_TRUE(ep.has_value());
  EXPECT_EQ(ep->episode, 3);
}

TEST(RssSeriesTracker, NoEpisodeReturnsNullopt) {
  auto ep = RssSeriesTracker::parseTitle(QStringLiteral("Just a random title with no episode"));
  EXPECT_FALSE(ep.has_value());
}

TEST(RssSeriesTracker, MatchesByName) {
  auto ep = RssSeriesTracker::parseTitle(QStringLiteral("My Show S01E05"));
  ASSERT_TRUE(ep.has_value());

  SeriesSubscription sub;
  sub.name = QStringLiteral("My Show");
  EXPECT_TRUE(RssSeriesTracker::matchesSeries(*ep, sub));
}

TEST(RssSeriesTracker, MatchesByAlias) {
  auto ep = RssSeriesTracker::parseTitle(QStringLiteral("Shingeki no Kyojin S04E01"));
  ASSERT_TRUE(ep.has_value());

  SeriesSubscription sub;
  sub.name = QStringLiteral("进击的巨人");
  sub.aliases = {QStringLiteral("Shingeki no Kyojin"), QStringLiteral("Attack on Titan")};
  EXPECT_TRUE(RssSeriesTracker::matchesSeries(*ep, sub));
}

TEST(RssSeriesTracker, IsNewerEpisode) {
  auto ep = RssSeriesTracker::parseTitle(QStringLiteral("Show S01E05"));
  ASSERT_TRUE(ep.has_value());

  SeriesSubscription sub;
  sub.name = QStringLiteral("Show");
  sub.season = 1;
  sub.last_episode_num = 4;
  EXPECT_TRUE(RssSeriesTracker::isNewerEpisode(*ep, sub));

  sub.last_episode_num = 5;
  EXPECT_FALSE(RssSeriesTracker::isNewerEpisode(*ep, sub));

  sub.last_episode_num = 6;
  EXPECT_FALSE(RssSeriesTracker::isNewerEpisode(*ep, sub));
}

TEST(RssSeriesTracker, QualityExtraction) {
  auto ep = RssSeriesTracker::parseTitle(QStringLiteral("Show S01E01 2160p HDR"));
  ASSERT_TRUE(ep.has_value());
  EXPECT_EQ(ep->quality, QStringLiteral("2160P"));
}

}  // namespace
