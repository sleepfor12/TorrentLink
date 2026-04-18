#ifndef PFD_CORE_BUILTIN_HTTP_TRACKER_H
#define PFD_CORE_BUILTIN_HTTP_TRACKER_H

#include <memory>

#include <QtCore/QObject>

namespace pfd::core {

/// 进程内嵌入式 HTTP Tracker（BEP 3 announce，compact IPv4）；监听 127.0.0.1，与 libtorrent 会话无关。
class BuiltinHttpTracker final : public QObject {
  Q_OBJECT

public:
  explicit BuiltinHttpTracker(QObject* parent = nullptr);
  ~BuiltinHttpTracker() override;

  BuiltinHttpTracker(const BuiltinHttpTracker&) = delete;
  BuiltinHttpTracker& operator=(const BuiltinHttpTracker&) = delete;

  /// 在主线程调用：按开关启停；port 为 0 时由系统分配端口。
  void apply(bool enabled, quint16 port);

  /// 当前监听端口；未监听时为 0。
  [[nodiscard]] quint16 serverPort() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace pfd::core

#endif  // PFD_CORE_BUILTIN_HTTP_TRACKER_H
