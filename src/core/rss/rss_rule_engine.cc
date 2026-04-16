#include "core/rss/rss_rule_engine.h"

#include <QtCore/QRegularExpression>

namespace pfd::core::rss {

namespace {

bool keywordMatch(const QString& text, const QString& keyword, bool use_regex) {
  if (use_regex) {
    QRegularExpression re(keyword, QRegularExpression::CaseInsensitiveOption);
    return re.isValid() && re.match(text).hasMatch();
  }
  return text.contains(keyword, Qt::CaseInsensitive);
}

}  // namespace

RuleMatchResult RssRuleEngine::evaluate(const RssRule& rule, const RssItem& item) {
  RuleMatchResult result;
  result.rule_id = rule.id;
  result.rule_name = rule.name;

  if (!rule.feed_ids.isEmpty() && !rule.feed_ids.contains(item.feed_id)) {
    result.matched = false;
    result.reason = QStringLiteral("订阅源不在规则范围内");
    return result;
  }

  const QString& title = item.title;

  bool includeOk = rule.include_keywords.isEmpty();
  QString matchedInclude;
  for (const auto& kw : rule.include_keywords) {
    if (keywordMatch(title, kw, rule.use_regex)) {
      includeOk = true;
      matchedInclude = kw;
      break;
    }
  }
  if (!includeOk) {
    result.matched = false;
    result.reason = QStringLiteral("未命中任何包含关键词");
    return result;
  }

  for (const auto& kw : rule.exclude_keywords) {
    if (keywordMatch(title, kw, rule.use_regex)) {
      result.matched = false;
      result.reason = QStringLiteral("命中排除关键词「%1」").arg(kw);
      return result;
    }
  }

  result.matched = true;
  if (matchedInclude.isEmpty()) {
    result.reason = QStringLiteral("规则「%1」命中（无包含词限制）").arg(rule.name);
  } else {
    result.reason = QStringLiteral("规则「%1」命中（包含词「%2」）").arg(rule.name, matchedInclude);
  }
  return result;
}

std::vector<RuleMatchResult> RssRuleEngine::evaluateAll(const std::vector<RssRule>& rules,
                                                        const RssItem& item) {
  std::vector<RuleMatchResult> results;
  results.reserve(rules.size());
  for (const auto& rule : rules) {
    results.push_back(evaluate(rule, item));
  }
  return results;
}

std::optional<RuleMatchResult>
RssRuleEngine::findFirstEnabledMatch(const std::vector<RssRule>& rules, const RssItem& item) {
  for (const auto& rule : rules) {
    if (!rule.enabled)
      continue;
    auto result = evaluate(rule, item);
    if (result.matched)
      return result;
  }
  return std::nullopt;
}

}  // namespace pfd::core::rss
