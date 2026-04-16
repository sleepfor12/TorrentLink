#ifndef PFD_CORE_RSS_RSS_OPML_H
#define PFD_CORE_RSS_RSS_OPML_H

#include <QtCore/QString>

#include <vector>

#include "core/rss/rss_types.h"

namespace pfd::core::rss {

class RssOpml {
public:
  struct ImportResult {
    std::vector<RssFeed> feeds;
    int skipped{0};
    QString error;
    [[nodiscard]] bool ok() const { return error.isEmpty(); }
  };

  [[nodiscard]] static ImportResult importFromFile(const QString& path);
  [[nodiscard]] static ImportResult importFromString(const QString& xml);

  [[nodiscard]] static bool exportToFile(const QString& path,
                                         const std::vector<RssFeed>& feeds);
  [[nodiscard]] static QString exportToString(const std::vector<RssFeed>& feeds);
};

}  // namespace pfd::core::rss

#endif  // PFD_CORE_RSS_RSS_OPML_H
