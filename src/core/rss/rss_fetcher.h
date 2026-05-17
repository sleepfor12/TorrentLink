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
  struct RequestHeaders {
    QString user_agent;
    QString accept_language;
    QString cookie_header;
    QString cookie_rules;
  };

  /// 与 libtorrent 会话代理一致；启用且 host 非空时用于 RSS / HTTP 拉取。
  struct HttpProxyConfig {
    bool enabled{false};
    QString type{QStringLiteral("socks5")};
    QString host;
    int port{1080};
    QString user;
    QString password;
  };

  RssFetcher() = default;

  void setRequestHeaders(RequestHeaders headers);
  void setHttpProxy(HttpProxyConfig proxy);

  [[nodiscard]] FetchResult fetch(const QString& url) const {
    return fetch(url, QString());
  }

  /// @param referer 非空时设置 HTTP Referer（防盗链站点常见需要）。
  [[nodiscard]] FetchResult fetch(const QString& url, const QString& referer) const;

private:
  RequestHeaders headers_;
  HttpProxyConfig proxy_;
};

}  // namespace pfd::core::rss

#endif  // PFD_CORE_RSS_RSS_FETCHER_H
