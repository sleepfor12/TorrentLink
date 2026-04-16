#include "core/rss/rss_opml.h"

#include <QtCore/QFile>
#include <QtCore/QUuid>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

namespace pfd::core::rss {

RssOpml::ImportResult RssOpml::importFromFile(const QString& path) {
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return {{}, 0, QStringLiteral("Cannot open file: %1").arg(path)};
  }
  return importFromString(QString::fromUtf8(f.readAll()));
}

RssOpml::ImportResult RssOpml::importFromString(const QString& xml) {
  ImportResult result;
  QXmlStreamReader reader(xml);

  while (!reader.atEnd()) {
    reader.readNext();
    if (reader.isStartElement() && reader.name() == QStringLiteral("outline")) {
      const auto attrs = reader.attributes();
      const QString xmlUrl = attrs.value(QStringLiteral("xmlUrl")).toString().trimmed();
      if (xmlUrl.isEmpty()) {
        ++result.skipped;
        continue;
      }
      RssFeed feed;
      feed.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
      feed.url = xmlUrl;
      feed.title = attrs.value(QStringLiteral("title")).toString().trimmed();
      if (feed.title.isEmpty()) {
        feed.title = attrs.value(QStringLiteral("text")).toString().trimmed();
      }
      if (feed.title.isEmpty()) feed.title = xmlUrl;
      result.feeds.push_back(std::move(feed));
    }
  }

  if (reader.hasError()) {
    result.error = QStringLiteral("XML parse error: %1").arg(reader.errorString());
  }
  return result;
}

bool RssOpml::exportToFile(const QString& path, const std::vector<RssFeed>& feeds) {
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
  const QString xml = exportToString(feeds);
  f.write(xml.toUtf8());
  return true;
}

QString RssOpml::exportToString(const std::vector<RssFeed>& feeds) {
  QString out;
  QXmlStreamWriter w(&out);
  w.setAutoFormatting(true);
  w.writeStartDocument();
  w.writeStartElement(QStringLiteral("opml"));
  w.writeAttribute(QStringLiteral("version"), QStringLiteral("2.0"));

  w.writeStartElement(QStringLiteral("head"));
  w.writeTextElement(QStringLiteral("title"), QStringLiteral("P2P File Downloader RSS Feeds"));
  w.writeEndElement(); // head

  w.writeStartElement(QStringLiteral("body"));
  for (const auto& feed : feeds) {
    w.writeStartElement(QStringLiteral("outline"));
    w.writeAttribute(QStringLiteral("type"), QStringLiteral("rss"));
    w.writeAttribute(QStringLiteral("text"), feed.title);
    w.writeAttribute(QStringLiteral("title"), feed.title);
    w.writeAttribute(QStringLiteral("xmlUrl"), feed.url);
    w.writeEndElement(); // outline
  }
  w.writeEndElement(); // body
  w.writeEndElement(); // opml
  w.writeEndDocument();
  return out;
}

}  // namespace pfd::core::rss
