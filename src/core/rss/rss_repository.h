#ifndef PFD_CORE_RSS_RSS_REPOSITORY_H
#define PFD_CORE_RSS_RSS_REPOSITORY_H

#include <QtCore/QString>

#include <vector>

#include "core/rss/rss_types.h"

namespace pfd::core::rss {

class RssRepository {
public:
  explicit RssRepository(QString data_dir);

  [[nodiscard]] std::vector<RssFeed> loadFeeds() const;
  [[nodiscard]] std::vector<RssItem> loadItems() const;
  [[nodiscard]] std::vector<RssRule> loadRules() const;
  [[nodiscard]] std::vector<SeriesSubscription> loadSeries() const;

  bool saveFeeds(const std::vector<RssFeed>& feeds) const;
  bool saveItems(const std::vector<RssItem>& items) const;
  bool saveRules(const std::vector<RssRule>& rules) const;
  bool saveSeries(const std::vector<SeriesSubscription>& series) const;

  [[nodiscard]] RssSettings loadSettings() const;
  bool saveSettings(const RssSettings& settings) const;

private:
  [[nodiscard]] QString feedsPath() const;
  [[nodiscard]] QString itemsPath() const;
  [[nodiscard]] QString rulesPath() const;
  [[nodiscard]] QString seriesPath() const;
  [[nodiscard]] QString settingsPath() const;

  QString data_dir_;
};

}  // namespace pfd::core::rss

#endif  // PFD_CORE_RSS_RSS_REPOSITORY_H
