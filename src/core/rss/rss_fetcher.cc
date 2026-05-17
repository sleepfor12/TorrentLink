#include "core/rss/rss_fetcher.h"

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <algorithm>
#include <utility>

namespace pfd::core::rss {

namespace {

bool isAllowedScheme(const QString& scheme) {
  return scheme == QStringLiteral("http") || scheme == QStringLiteral("https");
}

QString cookieFromRules(const QString& rulesText, const QString& host) {
  if (rulesText.trimmed().isEmpty() || host.trimmed().isEmpty()) {
    return {};
  }
  QStringList pairs;
  const QString hostLower = host.trimmed().toLower();
  for (const QString& raw : rulesText.split('\n', Qt::SkipEmptyParts)) {
    const QString line = raw.trimmed();
    if (line.isEmpty()) {
      continue;
    }
    const int sep = line.indexOf('\t');
    if (sep <= 0 || sep >= line.size() - 1) {
      continue;
    }
    const QString domain = line.left(sep).trimmed().toLower();
    const QString cookiePair = line.mid(sep + 1).trimmed();
    if (cookiePair.isEmpty()) {
      continue;
    }
    const bool matched =
        domain == QStringLiteral("*") || domain == hostLower ||
        (hostLower.size() > domain.size() && hostLower.endsWith(QStringLiteral(".") + domain));
    if (matched) {
      pairs.push_back(cookiePair);
    }
  }
  return pairs.join(QStringLiteral("; "));
}

void applyProxy(QNetworkAccessManager& nam, const RssFetcher::HttpProxyConfig& cfg) {
  if (cfg.enabled && !cfg.host.trimmed().isEmpty()) {
    const QString t = cfg.type.trimmed().toLower();
    const QNetworkProxy::ProxyType pt =
        t == QStringLiteral("http") ? QNetworkProxy::HttpProxy : QNetworkProxy::Socks5Proxy;
    const quint16 port = static_cast<quint16>(std::clamp(cfg.port, 1, 65535));
    nam.setProxy(QNetworkProxy(pt, cfg.host.trimmed(), port, cfg.user.trimmed(), cfg.password));
  } else {
    nam.setProxy(QNetworkProxy(QNetworkProxy::DefaultProxy));
  }
}

}  // namespace

void RssFetcher::setRequestHeaders(RequestHeaders headers) {
  headers_ = std::move(headers);
}

void RssFetcher::setHttpProxy(HttpProxyConfig proxy) {
  proxy_ = std::move(proxy);
}

FetchResult RssFetcher::fetch(const QString& url, const QString& referer) const {
  const QUrl parsed(url.trimmed());
  if (!parsed.isValid()) {
    return {false, 0, {}, QStringLiteral("Invalid URL: %1").arg(url)};
  }
  if (!isAllowedScheme(parsed.scheme().toLower())) {
    return {false,
            0,
            {},
            QStringLiteral("Scheme \"%1\" not allowed (http/https only)").arg(parsed.scheme())};
  }

  const QString refTrim = referer.trimmed();

  FetchResult result;
  for (int attempt = 0; attempt <= kFetchMaxRetries; ++attempt) {
    QNetworkAccessManager nam;
    applyProxy(nam, proxy_);
    QNetworkRequest req(parsed);
    req.setTransferTimeout(kFetchTimeoutMs);
    const QString userAgent = headers_.user_agent.trimmed().isEmpty()
                                  ? QStringLiteral("pfd-rss-reader/1.0")
                                  : headers_.user_agent.trimmed();
    req.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    req.setRawHeader(
        QByteArrayLiteral("Accept"),
        QByteArrayLiteral("application/rss+xml, application/atom+xml, application/xml, text/xml, "
                          "*/*;q=0.1"));
    if (!headers_.accept_language.trimmed().isEmpty()) {
      req.setRawHeader(QByteArrayLiteral("Accept-Language"),
                       headers_.accept_language.trimmed().toUtf8());
    }
    if (!headers_.cookie_header.trimmed().isEmpty()) {
      req.setRawHeader(QByteArrayLiteral("Cookie"), headers_.cookie_header.trimmed().toUtf8());
    }
    const QString ruleCookies = cookieFromRules(headers_.cookie_rules, parsed.host());
    if (!ruleCookies.isEmpty()) {
      const QString mergedCookies =
          headers_.cookie_header.trimmed().isEmpty()
              ? ruleCookies
              : QStringLiteral("%1; %2").arg(headers_.cookie_header.trimmed(), ruleCookies);
      req.setRawHeader(QByteArrayLiteral("Cookie"), mergedCookies.toUtf8());
    }
    if (!refTrim.isEmpty()) {
      req.setRawHeader(QByteArrayLiteral("Referer"), refTrim.toUtf8());
    }
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = nam.get(req);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(kFetchTimeoutMs + 2000);
    loop.exec();

    if (!reply->isFinished()) {
      reply->abort();
      reply->deleteLater();
      result = {false, 0, {}, QStringLiteral("Request timed out")};
      continue;
    }

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError) {
      result = {false, status, {}, QStringLiteral("Network error: %1").arg(reply->errorString())};
      reply->deleteLater();
      if (status >= 500) {
        continue;
      }
      return result;
    }

    const QByteArray body = reply->readAll();
    reply->deleteLater();

    if (body.size() > kFetchMaxBodyBytes) {
      return {false,
              status,
              {},
              QStringLiteral("Response too large (%1 bytes, max %2)")
                  .arg(body.size())
                  .arg(kFetchMaxBodyBytes)};
    }

    return {true, status, body, {}};
  }

  return result;
}

}  // namespace pfd::core::rss
