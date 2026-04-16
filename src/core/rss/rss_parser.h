#ifndef PFD_CORE_RSS_RSS_PARSER_H
#define PFD_CORE_RSS_RSS_PARSER_H

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <vector>

#include "core/rss/rss_types.h"

namespace pfd::core::rss {

struct ParseResult {
  bool ok{false};
  std::vector<RssItem> items;
  QString error;
};

class RssParser {
public:
  RssParser() = default;

  [[nodiscard]] ParseResult parse(const QString& feed_id, const QByteArray& xml) const;
};

}  // namespace pfd::core::rss

#endif  // PFD_CORE_RSS_RSS_PARSER_H
