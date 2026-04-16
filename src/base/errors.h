#ifndef PFD_BASE_ERRORS_H
#define PFD_BASE_ERRORS_H

#include <QtCore/QString>

#include <cstdint>
#include <system_error>

namespace pfd::base {

// 健壮错误码：显式底层类型，首值 kSuccess = 0，便于默认构造与序列化
enum class ErrorCode : std::uint32_t {
  kSuccess = 0,

  // Validation
  kInvalidMagnet,
  kInvalidTorrentPath,
  kInvalidSavePath,
  kSavePathNotWritable,
  kInvalidArgument,

  // Io
  kFileNotFound,
  kFileReadFailed,
  kFileWriteFailed,
  kDirectoryCreateFailed,
  kDiskFull,
  kPathPermissionDenied,

  // Torrent
  kAddTorrentFailed,
  kDuplicateTorrent,
  kMetadataTimeout,
  kTorrentRemoved,

  // Persistence
  kLoadTasksFailed,
  kSaveTasksFailed,
  kLoadResumeDataFailed,
  kSaveResumeDataFailed,

  // Unknown（未映射 / fromInt 越界）
  kUnknown,
};

enum class ErrorDomain : std::uint8_t {
  kNone = 0,
  kValidation,
  kIo,
  kSystem,
  kLibtorrent,
  kPersistence,
  kUnknown,
};

// 辅助函数：不依赖 Error 类，便于各处直接判断
[[nodiscard]] inline bool isSuccess(ErrorCode c) {
  return c == ErrorCode::kSuccess;
}

[[nodiscard]] std::uint32_t toInt(ErrorCode c);
[[nodiscard]] ErrorCode fromInt(std::uint32_t n);
[[nodiscard]] ErrorDomain domainForCode(ErrorCode c);

// 默认文案（message 为空时 UI 兜底）；可选后续接 i18n
[[nodiscard]] QString defaultMessage(ErrorCode c);

class Error {
public:
  Error() : code_(ErrorCode::kSuccess), domain_(ErrorDomain::kNone), raw_code_(0) {}

  Error(ErrorCode code, const QString& message, const QString& detail = {},
        ErrorDomain domain = ErrorDomain::kUnknown, int rawCode = 0)
      : code_(code), message_(message), detail_(detail),
        domain_(domain == ErrorDomain::kUnknown ? domainForCode(code) : domain),
        raw_code_(rawCode) {}

  [[nodiscard]] ErrorCode code() const {
    return code_;
  }
  [[nodiscard]] const QString& message() const {
    return message_;
  }
  [[nodiscard]] const QString& detail() const {
    return detail_;
  }
  [[nodiscard]] ErrorDomain domain() const {
    return domain_;
  }
  [[nodiscard]] int rawCode() const {
    return raw_code_;
  }

  [[nodiscard]] bool hasError() const {
    return !isSuccess(code_);
  }
  [[nodiscard]] bool isOk() const {
    return isSuccess(code_);
  }

  [[nodiscard]] QString displayMessage() const;

private:
  ErrorCode code_;
  QString message_;
  QString detail_;
  ErrorDomain domain_;
  int raw_code_;
};

// 与系统错误转换入口（Base 层统一入口；lt 错误转换在 lt 层实现）
[[nodiscard]] Error fromStdError(const std::error_code& ec, const QString& context = {});
[[nodiscard]] Error fromErrno(int err, const QString& context = {});

}  // namespace pfd::base

#endif  // PFD_BASE_ERRORS_H
