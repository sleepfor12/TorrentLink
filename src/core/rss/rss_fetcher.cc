#include "core/rss/rss_fetcher.h"

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace pfd::core::rss {

namespace {

bool isAllowedScheme(const QString& scheme) {
  return scheme == QStringLiteral("http") || scheme == QStringLiteral("https");
}

}  // namespace

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
    QNetworkRequest req(parsed);
    req.setTransferTimeout(kFetchTimeoutMs);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("pfd-rss-reader/1.0"));
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
