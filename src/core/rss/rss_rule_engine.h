#ifndef PFD_CORE_RSS_RSS_RULE_ENGINE_H
#define PFD_CORE_RSS_RSS_RULE_ENGINE_H

#include <optional>
#include <vector>

#include "core/rss/rss_types.h"

namespace pfd::core::rss {

class RssRuleEngine {
public:
  [[nodiscard]] static RuleMatchResult evaluate(const RssRule& rule, const RssItem& item);

  [[nodiscard]] static std::vector<RuleMatchResult> evaluateAll(const std::vector<RssRule>& rules,
                                                                const RssItem& item);

  [[nodiscard]] static std::optional<RuleMatchResult>
  findFirstEnabledMatch(const std::vector<RssRule>& rules, const RssItem& item);
};

}  // namespace pfd::core::rss

#endif  // PFD_CORE_RSS_RSS_RULE_ENGINE_H
