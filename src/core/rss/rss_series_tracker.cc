#include "core/rss/rss_series_tracker.h"

#include <QtCore/QRegularExpression>

namespace pfd::core::rss {

namespace {

QString extractQuality(const QString& title) {
  static const QRegularExpression re(
      QStringLiteral("(2160p|1080p|720p|480p|4K|UHD)"),
      QRegularExpression::CaseInsensitiveOption);
  const auto m = re.match(title);
  return m.hasMatch() ? m.captured(1).toUpper() : QString();
}

}  // namespace

std::optional<EpisodeInfo> RssSeriesTracker::parseTitle(const QString& title) {
  EpisodeInfo ep;
  ep.raw_title = title;
  ep.quality = extractQuality(title);

  // Pattern 1: S01E05 / S1E5
  {
    static const QRegularExpression re(
        QStringLiteral("(.+?)\\s*[\\[\\(]?S(\\d{1,2})E(\\d{1,4})[\\]\\)]?"),
        QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(title);
    if (m.hasMatch()) {
      ep.series_name = m.captured(1).trimmed();
      ep.season = m.captured(2).toInt();
      ep.episode = m.captured(3).toInt();
      return ep;
    }
  }

  // Pattern 2: [GroupName] Title - 05 [1080p]
  {
    static const QRegularExpression re(
        QStringLiteral("(?:\\[.+?\\]\\s*)?(.+?)\\s*-\\s*(\\d{1,4})\\s*(?:[\\[\\(v]|$)"));
    const auto m = re.match(title);
    if (m.hasMatch()) {
      bool ok = false;
      const int num = m.captured(2).toInt(&ok);
      if (ok && num > 0 && num < 2000) {
        ep.series_name = m.captured(1).trimmed();
        ep.episode = num;
        return ep;
      }
    }
  }

  // Pattern 3: 第05集 / 第5话
  {
    static const QRegularExpression re(
        QStringLiteral("(.+?)\\s*第\\s*(\\d{1,4})\\s*[集话期回]"));
    const auto m = re.match(title);
    if (m.hasMatch()) {
      ep.series_name = m.captured(1).trimmed();
      ep.episode = m.captured(2).toInt();
      return ep;
    }
  }

  // Pattern 4: EP05 / Ep.05
  {
    static const QRegularExpression re(
        QStringLiteral("(.+?)\\s*EP\\.?(\\d{1,4})"),
        QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(title);
    if (m.hasMatch()) {
      ep.series_name = m.captured(1).trimmed();
      ep.episode = m.captured(2).toInt();
      return ep;
    }
  }

  return std::nullopt;
}

bool RssSeriesTracker::matchesSeries(const EpisodeInfo& ep,
                                     const SeriesSubscription& sub) {
  const QString epName = ep.series_name.toLower().trimmed();

  if (sub.name.toLower().trimmed() == epName) return true;

  for (const auto& alias : sub.aliases) {
    if (alias.toLower().trimmed() == epName) return true;
    if (epName.contains(alias.toLower().trimmed())) return true;
  }

  if (sub.name.toLower().trimmed().isEmpty()) return false;
  return epName.contains(sub.name.toLower().trimmed());
}

bool RssSeriesTracker::isNewerEpisode(const EpisodeInfo& ep,
                                      const SeriesSubscription& sub) {
  if (sub.season >= 0 && ep.season >= 0 && ep.season != sub.season) {
    return ep.season > sub.season;
  }
  return ep.episode > sub.last_episode_num;
}

}  // namespace pfd::core::rss
