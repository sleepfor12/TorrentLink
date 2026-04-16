#ifndef PFD_CORE_RSS_RSS_SERIES_TRACKER_H
#define PFD_CORE_RSS_RSS_SERIES_TRACKER_H

#include <optional>

#include "core/rss/rss_types.h"

namespace pfd::core::rss {

class RssSeriesTracker {
public:
  [[nodiscard]] static std::optional<EpisodeInfo> parseTitle(const QString& title);

  [[nodiscard]] static bool matchesSeries(const EpisodeInfo& ep, const SeriesSubscription& sub);

  [[nodiscard]] static bool isNewerEpisode(const EpisodeInfo& ep, const SeriesSubscription& sub);
};

}  // namespace pfd::core::rss

#endif  // PFD_CORE_RSS_RSS_SERIES_TRACKER_H
