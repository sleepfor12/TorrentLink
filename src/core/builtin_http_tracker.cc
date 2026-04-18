#include "core/builtin_http_tracker.h"

#include <QtCore/QDateTime>
#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QRandomGenerator>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QUrlQuery>
#include <QtEndian>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include <algorithm>
#include <vector>

#include "core/bencode_minimal.h"
#include "core/logger.h"

namespace pfd::core {
namespace detail {

[[nodiscard]] QByteArray percentDecodeQueryValue(QByteArray enc) {
  QByteArray out;
  out.reserve(enc.size());
  for (int i = 0; i < enc.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(enc[i]);
    if (c == '%' && i + 2 < enc.size()) {
      const auto hex = [](unsigned char ch) -> int {
        if (ch >= '0' && ch <= '9') {
          return static_cast<int>(ch - '0');
        }
        if (ch >= 'A' && ch <= 'F') {
          return static_cast<int>(ch - 'A' + 10);
        }
        if (ch >= 'a' && ch <= 'f') {
          return static_cast<int>(ch - 'a' + 10);
        }
        return -1;
      };
      const int hi = hex(static_cast<unsigned char>(enc[i + 1]));
      const int lo = hex(static_cast<unsigned char>(enc[i + 2]));
      if (hi >= 0 && lo >= 0) {
        out.append(static_cast<char>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }
    if (c == '+') {
      out.append(' ');
      continue;
    }
    out.append(static_cast<char>(c));
  }
  return out;
}

[[nodiscard]] QByteArray queryParamRaw(const QByteArray& qs, const QByteArray& key) {
  const QByteArray needle = key + QByteArrayLiteral("=");
  int from = 0;
  while (from < qs.size()) {
    const int next = qs.indexOf('&', from);
    const QByteArray slice = next < 0 ? qs.mid(from) : qs.mid(from, next - from);
    if (slice.startsWith(needle)) {
      return percentDecodeQueryValue(slice.mid(needle.size()));
    }
    if (next < 0) {
      break;
    }
    from = next + 1;
  }
  return {};
}

constexpr int kAnnounceIntervalSec = 1800;
constexpr int kMinAnnounceIntervalSec = 900;
constexpr int kMaxPeersReturn = 50;
constexpr int kDefaultNumwant = 50;
constexpr qint64 kPeerTtlMs = 45LL * 60LL * 1000LL;

struct Peer {
  QByteArray peer_id;
  quint32 ipv4_host{};
  quint16 port{};
  qint64 left{};
  qint64 last_seen_ms{};
};

class PeerTable {
public:
  void upsert(const QByteArray& info_hash, const QByteArray& peer_id, quint32 ipv4_host,
              quint16 port, qint64 left, qint64 now_ms) {
    auto& vec = by_ih_[info_hash];
    for (auto& p : vec) {
      if (p.peer_id == peer_id) {
        p.ipv4_host = ipv4_host;
        p.port = port;
        p.left = left;
        p.last_seen_ms = now_ms;
        return;
      }
    }
    vec.push_back(Peer{peer_id, ipv4_host, port, left, now_ms});
  }

  void remove(const QByteArray& info_hash, const QByteArray& peer_id) {
    auto it = by_ih_.find(info_hash);
    if (it == by_ih_.end()) {
      return;
    }
    auto& vec = it.value();
    vec.erase(
        std::remove_if(vec.begin(), vec.end(), [&](const Peer& p) { return p.peer_id == peer_id; }),
        vec.end());
    if (vec.isEmpty()) {
      by_ih_.erase(it);
    }
  }

  void prune(qint64 now_ms) {
    for (auto it = by_ih_.begin(); it != by_ih_.end();) {
      auto& vec = it.value();
      vec.erase(
          std::remove_if(vec.begin(), vec.end(),
                         [&](const Peer& p) { return (now_ms - p.last_seen_ms) > kPeerTtlMs; }),
          vec.end());
      if (vec.isEmpty()) {
        it = by_ih_.erase(it);
      } else {
        ++it;
      }
    }
  }

  [[nodiscard]] int countComplete(const QByteArray& ih) const {
    const auto it = by_ih_.find(ih);
    if (it == by_ih_.end()) {
      return 0;
    }
    int n = 0;
    for (const auto& p : it.value()) {
      if (p.left == 0) {
        ++n;
      }
    }
    return n;
  }

  [[nodiscard]] int countIncomplete(const QByteArray& ih) const {
    const auto it = by_ih_.find(ih);
    if (it == by_ih_.end()) {
      return 0;
    }
    int n = 0;
    for (const auto& p : it.value()) {
      if (p.left > 0) {
        ++n;
      }
    }
    return n;
  }

  [[nodiscard]] QByteArray buildCompactPeers(const QByteArray& ih,
                                             const QByteArray& exclude_peer_id, int numwant) const {
    const auto it = by_ih_.find(ih);
    if (it == by_ih_.end()) {
      return {};
    }
    std::vector<Peer> cand;
    cand.reserve(static_cast<size_t>(it->size()));
    for (const auto& p : it.value()) {
      if (p.peer_id != exclude_peer_id) {
        cand.push_back(p);
      }
    }
    if (cand.empty()) {
      return {};
    }
    std::shuffle(cand.begin(), cand.end(), *QRandomGenerator::global());
    const int take = std::min({numwant, kMaxPeersReturn, static_cast<int>(cand.size())});
    QByteArray out;
    out.reserve(take * 6);
    for (int i = 0; i < take; ++i) {
      const Peer& p = cand[static_cast<size_t>(i)];
      quint32 be_ip = qToBigEndian(p.ipv4_host);
      out.append(reinterpret_cast<const char*>(&be_ip), 4);
      quint16 be_pt = qToBigEndian(p.port);
      out.append(reinterpret_cast<const char*>(&be_pt), 2);
    }
    return out;
  }

private:
  QHash<QByteArray, QVector<Peer>> by_ih_;
};

class TrackerWorker final : public QObject {
  Q_OBJECT

public:
  TrackerWorker() : server_(this) {
    connect(&server_, &QTcpServer::newConnection, this, &TrackerWorker::onNewConnection);
    cleanup_ = new QTimer(this);
    cleanup_->setInterval(60 * 1000);
    QObject::connect(cleanup_, &QTimer::timeout, this,
                     [this]() { table_.prune(QDateTime::currentMSecsSinceEpoch()); });
    cleanup_->start();
  }

  ~TrackerWorker() override = default;

public slots:
  void applyConfig(bool enabled, quint16 port) {
    if (server_.isListening()) {
      server_.close();
    }
    if (!enabled) {
      LOG_INFO(QStringLiteral("[builtin-tracker] stopped"));
      return;
    }
    const QHostAddress addr(QHostAddress::LocalHost);
    if (!server_.listen(addr, port)) {
      LOG_ERROR(QStringLiteral("[builtin-tracker] listen failed on 127.0.0.1:%1 (%2)")
                    .arg(port)
                    .arg(server_.errorString()));
      return;
    }
    LOG_INFO(QStringLiteral("[builtin-tracker] listening on http://127.0.0.1:%1/announce")
                 .arg(server_.serverPort()));
  }

  void shutdown() {
    cleanup_->stop();
    server_.close();
  }

  /// 仅在工作线程调用；跨线程请用 QMetaObject::invokeMethod(..., "queryPort", ...)。
  Q_INVOKABLE quint16 queryPort() const {
    return server_.isListening() ? static_cast<quint16>(server_.serverPort()) : 0;
  }

private:
  void onNewConnection() {
    while (server_.hasPendingConnections()) {
      auto* sock = server_.nextPendingConnection();
      QPointer<QTcpSocket> p(sock);
      QObject::connect(sock, &QTcpSocket::readyRead, this, [this, p]() {
        if (p.isNull()) {
          return;
        }
        handleRead(p.data());
      });
      QObject::connect(sock, &QTcpSocket::disconnected, this, [this, sock]() {
        read_buffers_.remove(sock);
        sock->deleteLater();
      });
    }
  }

  void handleRead(QTcpSocket* sock) {
    QByteArray buf = read_buffers_.value(sock);
    buf += sock->readAll();
    if (buf.size() > 1 << 20) {
      writeHttp(sock, 400, bencode::encodeAnnounceFailure(QStringLiteral("request too large")));
      read_buffers_.remove(sock);
      sock->close();
      return;
    }
    int hdr_end = buf.indexOf("\r\n\r\n");
    if (hdr_end < 0) {
      hdr_end = buf.indexOf("\n\n");
    }
    if (hdr_end < 0) {
      if (buf.size() > 8192) {
        writeHttp(sock, 400, bencode::encodeAnnounceFailure(QStringLiteral("request too large")));
        read_buffers_.remove(sock);
        sock->close();
      } else {
        read_buffers_[sock] = buf;
      }
      return;
    }
    read_buffers_.remove(sock);

    // hdr_end points at the first \r of "\r\n\r\n" (or '\n' of "\n\n"), so buf.left(hdr_end) omits
    // the request line's trailing CRLF; parse the first line from buf instead.
    int req_line_end = buf.indexOf("\r\n");
    if (req_line_end < 0 || req_line_end > hdr_end) {
      req_line_end = buf.indexOf('\n');
    }
    if (req_line_end <= 0 || req_line_end > hdr_end) {
      writeHttp(sock, 400, bencode::encodeAnnounceFailure(QStringLiteral("bad request")));
      return;
    }
    const QByteArray first = buf.left(req_line_end).trimmed();

    if (!first.startsWith("GET ")) {
      writeHttp(sock, 400, bencode::encodeAnnounceFailure(QStringLiteral("method not allowed")));
      return;
    }
    const int sp2 = first.indexOf(' ', 4);
    if (sp2 < 0) {
      writeHttp(sock, 400, bencode::encodeAnnounceFailure(QStringLiteral("bad request line")));
      return;
    }
    const QByteArray path_query = first.mid(4, sp2 - 4);
    if (!path_query.startsWith("/announce")) {
      writeHttp(sock, 400, bencode::encodeAnnounceFailure(QStringLiteral("unsupported path")));
      return;
    }
    const int qmark = path_query.indexOf('?');
    if (qmark < 0) {
      writeHttp(sock, 200, bencode::encodeAnnounceFailure(QStringLiteral("missing query")));
      return;
    }
    const QByteArray qs = path_query.mid(qmark + 1);
    QUrlQuery q;
    q.setQuery(QString::fromLatin1(qs));

    const QByteArray ih_raw = queryParamRaw(qs, QByteArrayLiteral("info_hash"));
    if (ih_raw.size() != 20) {
      writeHttp(sock, 200, bencode::encodeAnnounceFailure(QStringLiteral("invalid info_hash")));
      return;
    }

    const QByteArray peer_id = queryParamRaw(qs, QByteArrayLiteral("peer_id"));
    if (peer_id.size() != 20) {
      writeHttp(sock, 200, bencode::encodeAnnounceFailure(QStringLiteral("invalid peer_id")));
      return;
    }

    bool port_ok = false;
    const int port = q.queryItemValue(QStringLiteral("port")).toInt(&port_ok);
    if (!port_ok || port <= 0 || port > 65535) {
      writeHttp(sock, 200, bencode::encodeAnnounceFailure(QStringLiteral("invalid port")));
      return;
    }

    const QString compact_s = q.queryItemValue(QStringLiteral("compact"));
    if (!compact_s.isEmpty() && compact_s != QStringLiteral("1")) {
      writeHttp(sock, 200, bencode::encodeAnnounceFailure(QStringLiteral("compact=1 required")));
      return;
    }

    const qint64 left = q.queryItemValue(QStringLiteral("left")).toLongLong();

    int numwant = kDefaultNumwant;
    const QString nw = q.queryItemValue(QStringLiteral("numwant"));
    if (!nw.isEmpty()) {
      bool nw_ok = false;
      const int v = nw.toInt(&nw_ok);
      if (nw_ok && v >= 0) {
        numwant = std::min(v, kMaxPeersReturn);
      }
    }

    const QString ev = q.queryItemValue(QStringLiteral("event")).toLower();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    const QHostAddress peer_addr = sock->peerAddress();
    if (peer_addr.protocol() != QAbstractSocket::IPv4Protocol) {
      writeHttp(sock, 200, bencode::encodeAnnounceFailure(QStringLiteral("IPv6 not supported")));
      return;
    }
    const quint32 ipv4_host = peer_addr.toIPv4Address();

    if (ev == QStringLiteral("stopped")) {
      table_.remove(ih_raw, peer_id);
    } else {
      table_.upsert(ih_raw, peer_id, ipv4_host, static_cast<quint16>(port), left, now);
    }

    const int complete = table_.countComplete(ih_raw);
    const int incomplete = table_.countIncomplete(ih_raw);
    const QByteArray peers =
        table_.buildCompactPeers(ih_raw, peer_id, numwant > 0 ? numwant : kDefaultNumwant);
    const QByteArray body = bencode::encodeAnnounceOk(kAnnounceIntervalSec, kMinAnnounceIntervalSec,
                                                      complete, incomplete, peers);
    writeHttp(sock, 200, body);
  }

  static void writeHttp(QTcpSocket* s, int code, const QByteArray& body) {
    QByteArray hdr;
    hdr += "HTTP/1.0 ";
    hdr += QByteArray::number(code);
    hdr += (code == 200) ? " OK" : " Error";
    hdr += "\r\nContent-Type: text/plain\r\nContent-Length: ";
    hdr += QByteArray::number(body.size());
    hdr += "\r\n\r\n";
    s->write(hdr);
    s->write(body);
    s->flush();
    s->disconnectFromHost();
  }

  QTcpServer server_;
  PeerTable table_;
  QTimer* cleanup_{nullptr};
  QHash<QTcpSocket*, QByteArray> read_buffers_;
};

}  // namespace detail

struct BuiltinHttpTracker::Impl {
  QThread* thread{nullptr};
  detail::TrackerWorker* worker{nullptr};
};

BuiltinHttpTracker::BuiltinHttpTracker(QObject* parent)
    : QObject(parent), impl_(std::make_unique<Impl>()) {
  impl_->thread = new QThread(this);
  impl_->worker = new detail::TrackerWorker();
  impl_->worker->moveToThread(impl_->thread);
  QObject::connect(impl_->thread, &QThread::finished, impl_->worker, &QObject::deleteLater);
  impl_->thread->start();
}

BuiltinHttpTracker::~BuiltinHttpTracker() {
  if (impl_ && impl_->worker) {
    QMetaObject::invokeMethod(impl_->worker, "shutdown", Qt::BlockingQueuedConnection);
  }
  if (impl_ && impl_->thread) {
    impl_->thread->quit();
    impl_->thread->wait();
  }
}

void BuiltinHttpTracker::apply(bool enabled, quint16 port) {
  if (!impl_ || !impl_->worker) {
    return;
  }
  QMetaObject::invokeMethod(impl_->worker, "applyConfig", Qt::BlockingQueuedConnection,
                            Q_ARG(bool, enabled), Q_ARG(quint16, port));
}

quint16 BuiltinHttpTracker::serverPort() const {
  if (!impl_ || !impl_->worker) {
    return 0;
  }
  quint16 out = 0;
  QMetaObject::invokeMethod(impl_->worker, "queryPort", Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(quint16, out));
  return out;
}

}  // namespace pfd::core

#include "builtin_http_tracker.moc"
