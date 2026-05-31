#ifndef PFD_CORE_BUILTIN_HTTP_TRACKER_H
#define PFD_CORE_BUILTIN_HTTP_TRACKER_H

#include <QtCore/QObject>
#include <QtCore/QString>

#include <memory>

namespace pfd::core {

struct BuiltinTrackerConfig {
  bool enabled{false};
  quint16 port{0};
  /// `localhost`（127.0.0.1）或 `lan`（0.0.0.0，局域网可访问）
  QString bind_mode{QStringLiteral("localhost")};
};

/// 进程内嵌入式 HTTP Tracker（BEP 3 announce，compact IPv4）；与 libtorrent 会话无关。
class BuiltinHttpTracker final : public QObject {
  Q_OBJECT

public:
  explicit BuiltinHttpTracker(QObject* parent = nullptr);
  ~BuiltinHttpTracker() override;

  BuiltinHttpTracker(const BuiltinHttpTracker&) = delete;
  BuiltinHttpTracker& operator=(const BuiltinHttpTracker&) = delete;

  /// 在主线程调用：按配置启停；port 为 0 时由系统分配端口。
  void apply(const BuiltinTrackerConfig& cfg);

  /// 当前监听端口；未监听时为 0。
  [[nodiscard]] quint16 serverPort() const;

  /// 规范化后的绑定模式（`localhost` / `lan`）。
  [[nodiscard]] QString bindMode() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace pfd::core

#endif  // PFD_CORE_BUILTIN_HTTP_TRACKER_H
