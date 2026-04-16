#include "core/rss/rss_dedup.h"

#include <algorithm>

#include <QtCore/QRegularExpression>

#include "core/logger.h"

namespace pfd::core::rss {

QString RssDedup::extractInfoHash(const QString& magnet) {
  static const QRegularExpression re(
      QStringLiteral("xt=urn:btih:([0-9a-fA-F]{40})"),
      QRegularExpression::CaseInsensitiveOption);
  const auto m = re.match(magnet);
  if (m.hasMatch()) return m.captured(1).toLower();

  static const QRegularExpression re32(
      QStringLiteral("xt=urn:btih:([A-Za-z2-7]{32})"));
  const auto m32 = re32.match(magnet);
  if (m32.hasMatch()) return m32.captured(1).toLower();

  return {};
}

void RssDedup::buildIndex(const std::vector<RssItem>& existing) {
  known_ids_.clear();
  known_guids_.clear();
  known_links_.clear();
  known_infohashes_.clear();
  for (const auto& it : existing) {
    recordItem(it);
  }
  LOG_DEBUG(QStringLiteral("[rss.dedup] buildIndex existing=%1 ids=%2 guids=%3 links=%4 infohashes=%5")
                .arg(existing.size())
                .arg(known_ids_.size())
                .arg(known_guids_.size())
                .arg(known_links_.size())
                .arg(known_infohashes_.size()));
}

void RssDedup::recordItem(const RssItem& item) {
  if (!item.id.isEmpty()) known_ids_.insert(item.id);
  if (!item.guid.isEmpty()) known_guids_.insert(item.guid);
  if (!item.link.isEmpty()) known_links_.insert(item.link);
  if (!item.magnet.isEmpty()) {
    const auto ih = extractInfoHash(item.magnet);
    if (!ih.isEmpty()) known_infohashes_.insert(ih);
  }
}

bool RssDedup::isDuplicate(const RssItem& item) const {
  if (!item.id.isEmpty() && known_ids_.count(item.id) > 0) return true;

  if (!item.guid.isEmpty() && known_guids_.count(item.guid) > 0) return true;

  if (!item.link.isEmpty() && known_links_.count(item.link) > 0) return true;

  if (!item.magnet.isEmpty()) {
    const auto ih = extractInfoHash(item.magnet);
    if (!ih.isEmpty() && known_infohashes_.count(ih) > 0) return true;
  }
  return false;
}

std::vector<RssItem> RssDedup::filterDuplicates(
    const std::vector<RssItem>& existing,
    std::vector<RssItem>& incoming) {
  RssDedup dedup;
  dedup.buildIndex(existing);

  std::vector<RssItem> unique;
  unique.reserve(incoming.size());
  for (auto& item : incoming) {
    if (!dedup.isDuplicate(item)) {
      dedup.recordItem(item);
      unique.push_back(std::move(item));
    }
  }
  LOG_INFO(QStringLiteral("[rss.dedup] filterDuplicates existing=%1 incoming=%2 unique=%3 dropped=%4")
               .arg(existing.size())
               .arg(incoming.size())
               .arg(unique.size())
               .arg(static_cast<int>(incoming.size() - unique.size())));
  incoming.clear();
  return unique;
}

int RssDedup::pruneHistory(std::vector<RssItem>& items,
                           const HistoryPolicy& policy) {
  const auto now = QDateTime::currentDateTime();
  const int sizeBefore = static_cast<int>(items.size());

  if (policy.max_age_days > 0) {
    const auto cutoff = now.addDays(-policy.max_age_days);
    items.erase(std::remove_if(items.begin(), items.end(),
                               [&](const RssItem& it) {
                                 return it.published_at.isValid() &&
                                        it.published_at < cutoff &&
                                        !it.downloaded;
                               }),
                items.end());
  }

  if (policy.max_items > 0 && static_cast<int>(items.size()) > policy.max_items) {
    std::sort(items.begin(), items.end(),
              [](const RssItem& a, const RssItem& b) {
                if (a.downloaded != b.downloaded) return a.downloaded > b.downloaded;
                return a.published_at > b.published_at;
              });
    items.resize(static_cast<std::size_t>(policy.max_items));
  }

  const int removed = sizeBefore - static_cast<int>(items.size());
  if (removed > 0) {
    LOG_INFO(QStringLiteral("[rss.dedup] pruneHistory removed=%1 remaining=%2 max_items=%3 max_age_days=%4")
                 .arg(removed)
                 .arg(items.size())
                 .arg(policy.max_items)
                 .arg(policy.max_age_days));
  } else {
    LOG_DEBUG(QStringLiteral("[rss.dedup] pruneHistory no-op items=%1 max_items=%2 max_age_days=%3")
                  .arg(items.size())
                  .arg(policy.max_items)
                  .arg(policy.max_age_days));
  }
  return removed;
}

}  // namespace pfd::core::rss
