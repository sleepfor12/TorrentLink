#ifndef PFD_CORE_RSS_RSS_DEDUP_H
#define PFD_CORE_RSS_RSS_DEDUP_H

#include <unordered_set>
#include <vector>

#include <QtCore/QDateTime>

#include "core/rss/rss_types.h"

namespace pfd::core::rss {

struct HistoryPolicy {
  int max_items{5000};
  int max_age_days{90};
};

class RssDedup {
public:
  void buildIndex(const std::vector<RssItem>& existing);

  [[nodiscard]] bool isDuplicate(const RssItem& item) const;
  void recordItem(const RssItem& item);

  static std::vector<RssItem> filterDuplicates(
      const std::vector<RssItem>& existing,
      std::vector<RssItem>& incoming);

  static int pruneHistory(std::vector<RssItem>& items,
                          const HistoryPolicy& policy);

  static QString extractInfoHash(const QString& magnet);

private:
  std::unordered_set<QString> known_ids_;
  std::unordered_set<QString> known_guids_;
  std::unordered_set<QString> known_links_;
  std::unordered_set<QString> known_infohashes_;
};

}  // namespace pfd::core::rss

#endif  // PFD_CORE_RSS_RSS_DEDUP_H
