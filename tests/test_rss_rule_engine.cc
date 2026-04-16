#include <gtest/gtest.h>

#include "core/rss/rss_rule_engine.h"

namespace {

using pfd::core::rss::RssItem;
using pfd::core::rss::RssRule;
using pfd::core::rss::RssRuleEngine;

RssItem makeItem(const QString& title, const QString& feed_id = QStringLiteral("feed-1")) {
  RssItem it;
  it.id = QStringLiteral("item-1");
  it.feed_id = feed_id;
  it.title = title;
  it.magnet = QStringLiteral("magnet:?xt=urn:btih:abc");
  return it;
}

RssRule makeRule(const QStringList& include, const QStringList& exclude = {},
                 bool use_regex = false) {
  RssRule r;
  r.id = QStringLiteral("rule-1");
  r.name = QStringLiteral("Test Rule");
  r.include_keywords = include;
  r.exclude_keywords = exclude;
  r.use_regex = use_regex;
  r.enabled = true;
  return r;
}

TEST(RssRuleEngine, KeywordIncludeMatch) {
  auto rule = makeRule({QStringLiteral("1080p")});
  auto item = makeItem(QStringLiteral("My Show S01E01 1080p x264"));
  auto result = RssRuleEngine::evaluate(rule, item);
  EXPECT_TRUE(result.matched);
  EXPECT_FALSE(result.rule_name.isEmpty());
}

TEST(RssRuleEngine, KeywordIncludeNoMatch) {
  auto rule = makeRule({QStringLiteral("4K")});
  auto item = makeItem(QStringLiteral("My Show S01E01 1080p x264"));
  auto result = RssRuleEngine::evaluate(rule, item);
  EXPECT_FALSE(result.matched);
}

TEST(RssRuleEngine, KeywordExcludeOverridesInclude) {
  auto rule = makeRule({QStringLiteral("1080p")}, {QStringLiteral("x265")});
  auto item = makeItem(QStringLiteral("My Show S01E01 1080p x265"));
  auto result = RssRuleEngine::evaluate(rule, item);
  EXPECT_FALSE(result.matched);
}

TEST(RssRuleEngine, CaseInsensitiveKeyword) {
  auto rule = makeRule({QStringLiteral("hevc")});
  auto item = makeItem(QStringLiteral("My Show S01E01 HEVC 1080p"));
  auto result = RssRuleEngine::evaluate(rule, item);
  EXPECT_TRUE(result.matched);
}

TEST(RssRuleEngine, EmptyIncludeMatchesAll) {
  auto rule = makeRule({});
  auto item = makeItem(QStringLiteral("Anything at all"));
  auto result = RssRuleEngine::evaluate(rule, item);
  EXPECT_TRUE(result.matched);
}

TEST(RssRuleEngine, FeedScopeFiltering) {
  auto rule = makeRule({QStringLiteral("1080p")});
  rule.feed_ids = {QStringLiteral("feed-2")};
  auto item = makeItem(QStringLiteral("Title 1080p"), QStringLiteral("feed-1"));
  auto result = RssRuleEngine::evaluate(rule, item);
  EXPECT_FALSE(result.matched);
}

TEST(RssRuleEngine, RegexIncludeMatch) {
  auto rule = makeRule({QStringLiteral("S\\d{2}E\\d{2}")}, {}, true);
  auto item = makeItem(QStringLiteral("My Show S01E05 1080p"));
  auto result = RssRuleEngine::evaluate(rule, item);
  EXPECT_TRUE(result.matched);
}

TEST(RssRuleEngine, RegexExcludeMatch) {
  auto rule = makeRule({QStringLiteral("1080p")}, {QStringLiteral("x26[45]")}, true);
  auto item = makeItem(QStringLiteral("Title 1080p x265"));
  auto result = RssRuleEngine::evaluate(rule, item);
  EXPECT_FALSE(result.matched);
}

TEST(RssRuleEngine, EvaluateAllReturnsAllRules) {
  std::vector<RssRule> rules;
  rules.push_back(makeRule({QStringLiteral("1080p")}));
  auto r2 = makeRule({QStringLiteral("4K")});
  r2.id = QStringLiteral("rule-2");
  r2.name = QStringLiteral("4K Rule");
  rules.push_back(r2);

  auto item = makeItem(QStringLiteral("Show 1080p"));
  auto results = RssRuleEngine::evaluateAll(rules, item);
  ASSERT_EQ(results.size(), 2u);
  EXPECT_TRUE(results[0].matched);
  EXPECT_FALSE(results[1].matched);
}

TEST(RssRuleEngine, FindFirstEnabledMatchSkipsDisabled) {
  std::vector<RssRule> rules;
  auto r1 = makeRule({QStringLiteral("1080p")});
  r1.enabled = false;
  rules.push_back(r1);
  auto r2 = makeRule({QStringLiteral("1080p")});
  r2.id = QStringLiteral("rule-2");
  r2.name = QStringLiteral("Rule 2");
  r2.enabled = true;
  rules.push_back(r2);

  auto item = makeItem(QStringLiteral("Show 1080p"));
  auto match = RssRuleEngine::findFirstEnabledMatch(rules, item);
  ASSERT_TRUE(match.has_value());
  EXPECT_EQ(match->rule_id, QStringLiteral("rule-2"));
}

TEST(RssRuleEngine, FindFirstEnabledMatchReturnsNulloptWhenNone) {
  std::vector<RssRule> rules;
  auto r1 = makeRule({QStringLiteral("4K")});
  r1.enabled = true;
  rules.push_back(r1);

  auto item = makeItem(QStringLiteral("Show 1080p"));
  auto match = RssRuleEngine::findFirstEnabledMatch(rules, item);
  EXPECT_FALSE(match.has_value());
}

TEST(RssRuleEngine, ResultCarriesRuleName) {
  auto rule = makeRule({QStringLiteral("test")});
  rule.name = QStringLiteral("My Custom Rule");
  auto item = makeItem(QStringLiteral("test content"));
  auto result = RssRuleEngine::evaluate(rule, item);
  EXPECT_TRUE(result.matched);
  EXPECT_EQ(result.rule_name, QStringLiteral("My Custom Rule"));
}

}  // namespace
