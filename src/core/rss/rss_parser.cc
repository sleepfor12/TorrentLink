#include "core/rss/rss_parser.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtCore/QRegularExpression>
#include <QtCore/QUrl>
#include <QtCore/QXmlStreamReader>

namespace pfd::core::rss {

namespace {

QString stableItemId(const QString& feed_id, const QString& guid, const QString& link,
                     const QString& title) {
  if (!guid.isEmpty()) {
    return guid;
  }
  const QString seed = feed_id + QStringLiteral("|") + link + QStringLiteral("|") + title;
  return QString::fromLatin1(
      QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha256).toHex().left(32));
}

QString extractMagnetFromText(const QString& text) {
  static const QRegularExpression re(QStringLiteral("(magnet:\\?[^\\s\"<>]+)"));
  const auto m = re.match(text);
  return m.hasMatch() ? m.captured(1) : QString();
}

QDateTime parseDateTime(const QString& text) {
  QDateTime dt = QDateTime::fromString(text, Qt::RFC2822Date);
  if (dt.isValid())
    return dt;
  dt = QDateTime::fromString(text, Qt::ISODate);
  if (dt.isValid())
    return dt;
  dt = QDateTime::fromString(text, QStringLiteral("ddd, d MMM yyyy HH:mm:ss"));
  return dt;
}

bool isHttpOrHttpsScheme(const QString& scheme) {
  const QString s = scheme.toLower();
  return s == QStringLiteral("http") || s == QStringLiteral("https");
}

/// HTTP(S) URL whose path ends with .torrent (case-insensitive), query allowed.
bool looksLikeTorrentHttpUrl(const QString& urlText) {
  if (urlText.isEmpty())
    return false;
  const QUrl u(urlText);
  if (!u.isValid() || !isHttpOrHttpsScheme(u.scheme()))
    return false;
  const QString path = u.path();
  return path.endsWith(QStringLiteral(".torrent"), Qt::CaseInsensitive);
}

bool mimeTypeLooksLikeTorrent(const QString& mime) {
  const QString m = mime.toLower();
  return m.contains(QStringLiteral("bittorrent")) || m.contains(QStringLiteral("x-bittorrent"));
}

/// 从 HTML/纯文本正文中取第一个可信的 http(s) .torrent 链接（用于 description 内嵌 href）。
QString extractFirstTorrentHttpUrlFromText(const QString& text) {
  if (text.isEmpty())
    return {};
  static const QRegularExpression re(
      QStringLiteral(R"((https?://[^\s"'<>]+\.torrent(?:\?[^\s"'<>]*)?))"),
      QRegularExpression::CaseInsensitiveOption);
  const auto m = re.match(text);
  if (!m.hasMatch())
    return {};
  const QString u = m.captured(1);
  return looksLikeTorrentHttpUrl(u) ? u : QString();
}

void fillTorrentUrlFromBodyIfNeeded(RssItem& item) {
  if (!item.torrent_url.isEmpty())
    return;
  QString t = extractFirstTorrentHttpUrlFromText(item.summary);
  if (t.isEmpty()) {
    t = extractFirstTorrentHttpUrlFromText(item.link);
  }
  if (!t.isEmpty()) {
    item.torrent_url = t;
  }
}

// ─── RSS 2.0 ───

ParseResult parseRss2(const QString& feed_id, QXmlStreamReader& xml) {
  ParseResult result;
  result.ok = true;

  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isStartElement() && xml.name() == QStringLiteral("item")) {
      RssItem item;
      item.feed_id = feed_id;

      while (!(xml.isEndElement() && xml.name() == QStringLiteral("item"))) {
        xml.readNext();
        if (!xml.isStartElement())
          continue;

        const auto tag = xml.name();
        if (tag == QStringLiteral("title")) {
          item.title = xml.readElementText().trimmed();
        } else if (tag == QStringLiteral("link")) {
          const QString text = xml.readElementText().trimmed();
          if (text.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) {
            item.magnet = text;
          } else if (looksLikeTorrentHttpUrl(text)) {
            item.torrent_url = text;
            item.link = text;
          } else {
            item.link = text;
          }
        } else if (tag == QStringLiteral("guid")) {
          item.guid = xml.readElementText().trimmed();
        } else if (tag == QStringLiteral("description")) {
          item.summary = xml.readElementText().trimmed();
        } else if (tag == QStringLiteral("pubDate")) {
          item.published_at = parseDateTime(xml.readElementText().trimmed());
        } else if (tag == QStringLiteral("enclosure")) {
          const QString url = xml.attributes().value(QStringLiteral("url")).toString().trimmed();
          const QString encType =
              xml.attributes().value(QStringLiteral("type")).toString().trimmed();
          const bool torrentByMime = mimeTypeLooksLikeTorrent(encType);
          if (url.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) {
            item.magnet = url;
          } else if (url.endsWith(QStringLiteral(".torrent"), Qt::CaseInsensitive) ||
                     torrentByMime || looksLikeTorrentHttpUrl(url)) {
            if (item.torrent_url.isEmpty()) {
              item.torrent_url = url;
            }
          }
          xml.skipCurrentElement();
        } else {
          xml.skipCurrentElement();
        }
      }

      if (item.magnet.isEmpty()) {
        QString candidate = extractMagnetFromText(item.link);
        if (candidate.isEmpty())
          candidate = extractMagnetFromText(item.summary);
        if (!candidate.isEmpty())
          item.magnet = candidate;
      }

      fillTorrentUrlFromBodyIfNeeded(item);

      item.id = stableItemId(feed_id, item.guid, item.link, item.title);
      result.items.push_back(std::move(item));
    }
  }

  if (xml.hasError()) {
    result.error = xml.errorString();
    if (result.items.empty()) {
      result.ok = false;
    }
  }
  return result;
}

// ─── Atom ───

ParseResult parseAtom(const QString& feed_id, QXmlStreamReader& xml) {
  ParseResult result;
  result.ok = true;

  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isStartElement() && xml.name() == QStringLiteral("entry")) {
      RssItem item;
      item.feed_id = feed_id;

      while (!(xml.isEndElement() && xml.name() == QStringLiteral("entry"))) {
        xml.readNext();
        if (!xml.isStartElement())
          continue;

        const auto tag = xml.name();
        if (tag == QStringLiteral("title")) {
          item.title = xml.readElementText().trimmed();
        } else if (tag == QStringLiteral("link")) {
          const QString href = xml.attributes().value(QStringLiteral("href")).toString().trimmed();
          const QString linkType =
              xml.attributes().value(QStringLiteral("type")).toString().trimmed();
          const bool torrentByMime = mimeTypeLooksLikeTorrent(linkType);
          if (href.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) {
            item.magnet = href;
          } else if (href.endsWith(QStringLiteral(".torrent"), Qt::CaseInsensitive) ||
                     torrentByMime || looksLikeTorrentHttpUrl(href)) {
            if (item.torrent_url.isEmpty()) {
              item.torrent_url = href;
            }
            if (item.link.isEmpty()) {
              item.link = href;
            }
          } else if (item.link.isEmpty()) {
            item.link = href;
          }
          if (!xml.isEndElement())
            xml.skipCurrentElement();
        } else if (tag == QStringLiteral("id")) {
          item.guid = xml.readElementText().trimmed();
        } else if (tag == QStringLiteral("summary") || tag == QStringLiteral("content")) {
          item.summary = xml.readElementText().trimmed();
        } else if (tag == QStringLiteral("published") || tag == QStringLiteral("updated")) {
          if (!item.published_at.isValid()) {
            item.published_at = parseDateTime(xml.readElementText().trimmed());
          } else {
            xml.skipCurrentElement();
          }
        } else {
          xml.skipCurrentElement();
        }
      }

      if (item.magnet.isEmpty()) {
        QString candidate = extractMagnetFromText(item.link);
        if (candidate.isEmpty())
          candidate = extractMagnetFromText(item.summary);
        if (!candidate.isEmpty())
          item.magnet = candidate;
      }

      fillTorrentUrlFromBodyIfNeeded(item);

      item.id = stableItemId(feed_id, item.guid, item.link, item.title);
      result.items.push_back(std::move(item));
    }
  }

  if (xml.hasError()) {
    result.error = xml.errorString();
    if (result.items.empty()) {
      result.ok = false;
    }
  }
  return result;
}

}  // namespace

ParseResult RssParser::parse(const QString& feed_id, const QByteArray& xml_data) const {
  if (xml_data.isEmpty()) {
    return {false, {}, QStringLiteral("Empty XML data")};
  }

  QXmlStreamReader xml(xml_data);

  while (!xml.atEnd()) {
    xml.readNext();
    if (!xml.isStartElement())
      continue;

    const auto root = xml.name();
    if (root == QStringLiteral("rss") || root == QStringLiteral("channel")) {
      return parseRss2(feed_id, xml);
    }
    if (root == QStringLiteral("feed")) {
      return parseAtom(feed_id, xml);
    }
  }

  return {false, {}, QStringLiteral("Unrecognized feed format (not RSS 2.0 or Atom)")};
}

}  // namespace pfd::core::rss
