#ifndef PFD_CORE_RSS_RSS_FETCHER_H
#define PFD_CORE_RSS_RSS_FETCHER_H

#include <QtCore/QByteArray>
#include <QtCore/QString>

namespace pfd::core::rss {

inline constexpr int kFetchTimeoutMs = 15000;
inline constexpr int kFetchMaxRetries = 2;
inline constexpr qint64 kFetchMaxBodyBytes = 10 * 1024 * 1024;  // 10 MiB

struct FetchResult {
  bool ok{false};
  int http_status{0};
  QByteArray body;
  QString error;
};

class RssFetcher {
public:
  RssFetcher() = default;

  [[nodiscard]] FetchResult fetch(const QString& url) const {
    return fetch(url, QString());
  }

  /// @param referer 非空时设置 HTTP Referer（防盗链站点常见需要）。
  [[nodiscard]] FetchResult fetch(const QString& url, const QString& referer) const;
};

}  // namespace pfd::core::rss

#endif  // PFD_CORE_RSS_RSS_FETCHER_H
